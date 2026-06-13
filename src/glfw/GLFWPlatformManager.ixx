/**
 * @file GLFWPlatformManager.ixx
 * @brief GLFW-backed platform manager handling runtime init, window lifecycle, and frame service commands.
 */
module;

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <ostream>
#include <ranges>
#include <vector>

export module helios.glfw.GLFWPlatformManager;

import helios.engine.runtime.world.UpdateContext;
import helios.engine.runtime.world.Session;

import helios.engine.util.log;
import helios.engine.core.types;

import helios.engine.runtime.enginestate.types;

import helios.engine.spatial.components;

import helios.engine.rendering.renderTarget;

import helios.engine.runtime.messaging.command.concepts.IsPlatformCommandBuffer;
import helios.engine.runtime.messaging.command.CommandHandlerRegistry;
import helios.engine.runtime.messaging.command.CommandBufferRegistry;

import helios.engine.state.commands;
import helios.engine.state.types;

import helios.engine.runtime.world.EngineWorld;
import helios.engine.runtime.world.tags.ManagerRole;

import helios.engine.platform.environment.commands;
import helios.engine.platform.lifecycle.commands;
import helios.engine.platform.environment.components;
import helios.engine.platform.environment.types;

import helios.engine.platform.window.commands;
import helios.engine.platform.window.components;
import helios.engine.platform.window.types;

import helios.glfw.components;
import helios.glfw.types;

import helios.engine.rendering.common.concepts.CanProvideWindowHints;
import helios.engine.rendering.common.concepts.CanInitializeRenderBackend;

import helios.engine.runtime.concepts;
import helios.engine.runtime.messaging.command;
import helios.engine.platform.window.concepts.IsWindowHandle;

using namespace helios::engine::rendering::renderTarget::types;
using namespace helios::engine::rendering::renderTarget::components;
using namespace helios::engine::spatial::components;
using namespace helios::engine::runtime::world::tags;
using namespace helios::engine::platform::environment::commands;
using namespace helios::engine::platform::lifecycle::commands;
using namespace helios::engine::platform::environment::types;
using namespace helios::engine::platform::environment::components;
using namespace helios::engine::platform::window::commands;
using namespace helios::engine::platform::window::types;
using namespace helios::engine::platform::window::components;
using namespace helios::glfw::components;
using namespace helios::glfw::types;
using namespace helios::engine::state::commands;
using namespace helios::engine::state::types;
using namespace helios::engine::runtime::messaging::command::concepts;
using namespace helios::engine::rendering::common::concepts;
using namespace helios::engine::core::types;
using namespace helios::engine::runtime::messaging::command;
using namespace helios::engine::runtime::world;
using namespace helios::engine::platform::window::concepts;
using namespace helios::engine::runtime::enginestate::types;

#define HELIOS_LOG_SCOPE "helios::glfw::GLFWPlatformManager"
export namespace helios::glfw {

    /**
     * @brief Concrete manager integrating GLFW with helios runtime/window command flow.
     *
     * The manager receives runtime and window commands via `submit(...)`, stores them
     * as pending work, and processes them in `flush(...)` in a deterministic order.
     *
     * @tparam THandle Window/entity handle type.
     * @tparam TStateCommandBuffer Command buffer used for follow-up state commands.
     * @tparam TPlatformCommandBuffer Command buffer used by GLFW callbacks for platform commands.
     */
    template<typename TRenderPlatform, typename THandle, typename TStateCommandBuffer = NullCommandBuffer, typename TPlatformCommandBuffer = NullCommandBuffer>
    requires IsWindowHandle<THandle>
            && IsCommandBufferLike<TStateCommandBuffer>
            && IsPlatformCommandBuffer<TPlatformCommandBuffer>
            && CanInitializeRenderBackend<TRenderPlatform>
            && CanProvideWindowHints<TRenderPlatform>
    class GLFWPlatformManager {

        std::vector<WindowResizeCommand<THandle>> pendingResizeCommands_;

        std::vector<WindowCreateCommand<THandle>> windowCreateCommands_;

        std::vector<SwapBuffersCommand<THandle>> pendingBufferSwaps_;

        std::vector<WindowCloseCommand<THandle>> pendingCloseCommands_;

        std::vector<THandle> currentContexts_;

        bool shouldInit_ = false;

        bool shouldShutdown_ = false;

        bool pollEvents_ = false;

        bool initialized_ = false;

        TRenderPlatform& renderPlatform_;

        inline static const helios::engine::util::log::Logger& logger_ = helios::engine::util::log::LogManager::loggerForScope(
                   HELIOS_LOG_SCOPE);

        PlatformWorld* platformWorld_;

        /**
         * @brief Initializes GLFW and transitions runtime/session from booting to boot request.
         *
         * @param updateContext Frame-local update context.
         */
        bool initPlatform(UpdateContext& updateContext) noexcept {

            if (!shouldInit_ || initialized_) {
                return false;
            }

            if (glfwInit() == GLFW_FALSE) {
                assert(false && "Failed to initialize glfw");
            }

            renderPlatform_.provideWindowHints();

            assert(updateContext.session().state<EngineState>() == EngineState::Booting &&
                "Expected EngineState to be Booting during platform initialization");

            initialized_ = updateContext.session().initialize() &&
                           updateContext.runtimeEnvironment().initialize();
            
            shouldInit_ = false;
            
            return initialized_;
        }


        /**
         * @brief Creates a native GLFW window and binds it to the requested window entity.
         *
         * @param updateContext Frame-local update context.
         * @param cmd Window creation command containing target handle and config.
         *
         * @return `true` if the window was created and bound successfully; otherwise `false`.
         */
        bool createWindow(UpdateContext& updateContext, const WindowCreateCommand<THandle>& cmd) noexcept {

            auto window = updateContext.find(cmd.windowHandle);

            if (!window) {
                return false;
            }

            auto& cfg = cmd.windowConfig;

            auto* nativeHandle = glfwCreateWindow(
                cfg.size[0],
                cfg.size[1],
                cfg.title.c_str(),
                nullptr,
                nullptr
            );

            assert(nativeHandle && "Failed to create GLFW window");

            if (cfg.aspectRatioNumer > 0 && cfg.aspectRatioDenom > 0) {
                glfwSetWindowAspectRatio(
                    nativeHandle,
                    cfg.aspectRatioNumer,
                    cfg.aspectRatioDenom
                );
            }

            assert(window->template has<WindowCreateRequestComponent<THandle>>() && "Expected entity to have WindowCreateRequestComponent");
            window->template remove<WindowCreateRequestComponent<THandle>>();
            assert(!window->template has<WindowCreateRequestComponent<THandle>>() && "Expected entity to not have WindowCreateRequestComponent");
            assert(!window->template has<WindowComponent<THandle>>() && "Expected entity to not have WindowComponent");
            window->template add<WindowComponent<THandle>>(
                std::move(cfg.title),
                cfg.aspectRatioNumer,
                cfg.aspectRatioDenom
            );
            window->template add<
                Size2DComponent<THandle>
            >(WindowSize(cfg.size));
            window->template add<GLFWWindowHandleComponent<THandle>>(nativeHandle);

            removeCurrentContext(updateContext);

            glfwMakeContextCurrent(nativeHandle);

            window->template add<CurrentContextComponent<THandle>>();

            window->template add<WindowShownComponent<THandle>>();
            window->template add<GLFWWindowUserPointerComponent<THandle, TPlatformCommandBuffer>>(
                GLFWWindowUserPointer<THandle, TPlatformCommandBuffer>(
                    cmd.windowHandle, commandBufferRegistry_->template item<TPlatformCommandBuffer>()
            ));

            installResizeListener(cmd.windowHandle);


            int renderTargetWidth = 0;
            int renderTargetHeight = 0;
            int windowWidth = 0;
            int windowHeight = 0;

            glfwGetFramebufferSize(nativeHandle, &renderTargetWidth, &renderTargetHeight);
            glfwGetWindowSize(nativeHandle, &windowWidth, &windowHeight);

            commandBufferRegistry_->template item<TPlatformCommandBuffer>()
                                  ->template add<WindowResizeCommand<THandle>>(
                                    cmd.windowHandle,
                                    WindowSize(windowWidth, windowHeight),
                                    RenderTargetSize(renderTargetWidth, renderTargetHeight));

            return true;
        }

        /**
         * @brief Removes `CurrentContextComponent` from all windows currently marked as active context.
         *
         * @param updateContext Frame-local update context.
         */
        void removeCurrentContext(UpdateContext& updateContext) {
            // remove any currentcontext component
            currentContexts_.clear();
            for (auto [window, cc]: updateContext.template view<THandle, CurrentContextComponent<THandle>>()) {
                currentContexts_.push_back(window.handle());
            }
            for (auto& handle : currentContexts_) {
                auto go = updateContext.find<THandle> (handle);
                if (go) {
                    go->template remove<CurrentContextComponent<THandle>>();
                }
            }
        }

        /**
         * @brief Installs GLFW user-pointer data and renderTarget resize callback for a window.
         *
         * @param handle Window handle for which the listener is installed.
         */
        void installResizeListener(THandle handle) noexcept {

            auto entity = platformWorld_->findEntity<THandle>(handle);

            if (!entity) {
                logger_.warn("Entity was not found");
                return;
            }

            const auto* glfw = entity->template get<GLFWWindowHandleComponent<THandle>>();
            if (!glfw) {
                logger_.error("Entity does not have GLFWWindowHandleComponent");
                assert(false && "Entity does not have GLFWWindowHandleComponent");
                return;
            }

            auto* wuptrComponent = entity->template get<GLFWWindowUserPointerComponent<THandle, TPlatformCommandBuffer>>();
            if (!wuptrComponent) {
                logger_.error("Entity does not have GLFWWindowUserPointerComponent");
                assert(false && "Entity does not have GLFWWindowUserPointerComponent");
                return;
            }
            auto* wuptr =  &wuptrComponent->userPointer;

            glfwSetWindowUserPointer(glfw->handle, static_cast<void*>(wuptr));

            glfwSetFramebufferSizeCallback(
                glfw->handle,
                [] (GLFWwindow* nativeHandle, const int width, const int height) {
                const auto* ptr = static_cast<GLFWWindowUserPointer<THandle, TPlatformCommandBuffer>*>(glfwGetWindowUserPointer(nativeHandle));

                if (ptr && ptr->platformCommandBuffer) {

                    int windowWidth = 0;
                    int windowHeight = 0;

                    glfwGetWindowSize(nativeHandle, &windowWidth, &windowHeight);
                    ptr->platformCommandBuffer->template add<WindowResizeCommand<THandle>>(
                        ptr->windowHandle,
                        WindowSize(windowWidth, windowHeight),
                        RenderTargetSize(width, height)
                    );
                }
            });
        }


        /**
         * @brief Executes a single swap-buffers command.
         *
         * @param updateContext Frame-local update context.
         * @param cmd Swap-buffers command.
         */
        void swapBuffer(UpdateContext& updateContext, const SwapBuffersCommand<THandle>& cmd) noexcept {

            const auto entity = updateContext.find(cmd.windowHandle);

            if (!entity) {
                logger_.warn("Entity was not found");
                return;
            }

            const auto* glfw = entity->template get<GLFWWindowHandleComponent<THandle>>();

            if (!glfw) {
                logger_.error("Entity does not have GLFWWindowHandleComponent");
                assert(false && "Entity does not have GLFWWindowHandleComponent");
                return;
            }

            assert((updateContext.session().state<EngineState>() != EngineState::Booting) &&
                "GLFWSwapBuffersSystem should not be running during boot");
            assert(glfw->handle && "GLFWWindowComponent has no native handle");
            glfwSwapBuffers(glfw->handle);
        }


        /**
         * @brief Creates all windows queued during command submission.
         *
         * @param updateContext Frame-local update context.
         *
         * @return `true` if at least one window was created in this flush; otherwise `false`.
         */
        bool createWindows(UpdateContext& updateContext) noexcept {
            if (windowCreateCommands_.empty()) {
                return false;
            }
            for (const auto& windowCreateCommand  : windowCreateCommands_) {
                const bool isContextAvailable = createWindow(updateContext, windowCreateCommand);
                if (!isContextAvailable) {
                    logger_.error("Failed to create window");
                    assert(false && "Failed to create window");
                }
            }

            windowCreateCommands_.clear();
            return true;
        }

        /**
         * @brief Applies queued resize commands to window components.
         *
         * @details Applies queued resize commands to window components.
         * This will also affect the underlying renderTargets, for as long
         * as the specific windows are bound to a renderTarget.
         *
         * @param updateContext Frame-local update context.
         */
        void resizeWindows(UpdateContext& updateContext) noexcept {

            if (pendingResizeCommands_.empty()) {
                return;
            }

            for (const auto& [windowHandle, windowSize, renderTargetSize]: pendingResizeCommands_) {

                if (!windowHandle.isValid()) {
                    continue;
                }

                if (auto entity = updateContext.find(windowHandle)) {

                    if (auto* wsc = entity->template get<Size2DComponent<THandle>>()) {
                        wsc->setValue(windowSize);
                    }
                    if (auto* fbc =  entity->template get<RenderTargetBindingComponent<THandle>>()) {
                        auto renderTargetHandle = fbc->targetHandle();
                        auto renderTarget = updateContext.find<RenderTargetHandle>(renderTargetHandle);
                        auto fsc = renderTarget->template get<Size2DComponent<RenderTargetHandle>>();

                        logger_.info("Setting renderTarget size to {0},{1}", renderTargetSize[0], renderTargetSize[1]);
                        fsc->setValue(renderTargetSize);
                    }
                }

            }

            pendingResizeCommands_.clear();
        }

        /**
         * @brief Processes all queued swap-buffers commands.
         *
         * @param updateContext Frame-local update context.
         */
        void swapBuffers(UpdateContext& updateContext) noexcept {
            if (pendingBufferSwaps_.empty()) {
                return;
            }
            for (const auto& swapBufferCommand : pendingBufferSwaps_) {
                swapBuffer(updateContext, swapBufferCommand);
            }
            pendingBufferSwaps_.clear();
        }

        /**
         * @brief Polls GLFW events if requested by a poll-events command.
         *
         * @param updateContext Frame-local update context.
         */
        void pollEvents(UpdateContext& updateContext) noexcept {
            if (!pollEvents_) {
                return;
            }

            glfwPollEvents();
            pollEvents_ = false;
        }

        /**
         * @brief Destroys all windows queued for closing.
         *
         * @param updateContext Frame-local update context.
         */
        void closeWindows(UpdateContext& updateContext) noexcept {
            if (pendingCloseCommands_.empty()) {
                return;
            }

            for (const auto& cmd : pendingCloseCommands_) {
                auto entity = updateContext.find(cmd.windowHandle);

                if (!entity) {
                    logger_.warn("Entity was not found");
                    continue;
                }

                const auto* glfw = entity->template get<GLFWWindowHandleComponent<THandle>>();
                if (!glfw) {
                    logger_.warn("Entity does not have GLFWWindowHandleComponent");
                    continue;
                }

                glfwDestroyWindow(glfw->handle);
                bool destroyed = platformWorld_->destroy<THandle>(cmd.windowHandle);
                assert(destroyed && "Failed to destroy entity");
            }

            pendingCloseCommands_.clear();
        }


        /**
         * @brief Terminates GLFW and queues a shutdown state transition request.
         *
         * @param updateContext Frame-local update context.
         */
        void shutdown(UpdateContext& updateContext) noexcept {

            glfwTerminate();

            commandBufferRegistry_->template item<TStateCommandBuffer>()->template add<StateCommand<EngineState>>(
               StateTransitionRequest<EngineState>(
                   updateContext.session().state<EngineState>(),
                   EngineStateTransitionId::ShutdownRequest
               )
           );


        }

        CommandBufferRegistry* commandBufferRegistry_ = nullptr;
        
        public:


        /**
         * @brief Engine role marker used by runtime registries.
         */
        using EngineRoleTag = ManagerRole;

        explicit GLFWPlatformManager(
            TRenderPlatform& renderPlatform,
            PlatformWorld& platformWorld,
            CommandBufferRegistry& commandBufferRegistry)
        : renderPlatform_(renderPlatform),
          platformWorld_(&platformWorld),
          commandBufferRegistry_(&commandBufferRegistry) {};
        

        /**
         * @brief Processes queued platform/window work for the current frame.
         *
         * @param updateContext Frame-local update context.
         */
        void flush(UpdateContext& updateContext)  noexcept {

            if (shouldShutdown_) {
                shutdown(updateContext);
                return;
            }

            if (initPlatform(updateContext)) {
                commandBufferRegistry_->template item<TStateCommandBuffer>()->template add<StateCommand<EngineState>>(
                StateTransitionRequest<EngineState>(
                    updateContext.session().state<EngineState>(),
                    EngineStateTransitionId::BootRequest
                )
            );
            }
            pollEvents(updateContext);
            const bool isContextAvailable = createWindows(updateContext);

            if (!renderPlatform_.isInitialized() && isContextAvailable) {
                if (renderPlatform_.init()) {
                    updateContext.runtimeEnvironment().setGPUReady();
                }
            }

            resizeWindows(updateContext);
            closeWindows(updateContext);
            swapBuffers(updateContext);
        }

        /**
         * @brief Marks that platform events should be polled in the next `flush(...)`.
         *
         * @param command Poll-events marker command.
         *
         * @return `true` when the command was accepted.
         */
        bool submit(PollEventsCommand&& command) noexcept {
            pollEvents_ = true;
            return true;
        }

        /**
         * @brief Marks platform initialization for execution during the next `flush(...)`.
         *
         * @param command Platform-init marker command.
         *
         * @return `true` when the command was accepted.
         */
        bool submit(PlatformInitCommand&& command) noexcept {
            assert(!initialized_ && "Application was already initialized.");
            shouldInit_ = true;
            return true;
        }

        /**
         * @brief Queues a window creation request.
         *
         * @param command Window creation command.
         *
         * @return `true` when the command was queued.
         */
        bool submit(WindowCreateCommand<THandle>&& command)  noexcept {
            windowCreateCommands_.push_back(std::move(command));
            return true;
        }

        /**
         * @brief Queues a buffer-swap request for a specific window.
         *
         * @param command Swap-buffers command.
         *
         * @return `true` when the command was queued.
         */
        bool submit(SwapBuffersCommand<THandle>&& command)  noexcept {
            pendingBufferSwaps_.push_back(std::move(command));
            return true;
        }

        /**
         * @brief Stores the latest resize request per window entity index.
         *
         * @param command Window resize command.
         *
         * @return `true` when the command was stored.
         */
        bool submit(WindowResizeCommand<THandle>&& command)  noexcept {
            const auto idx = command.windowHandle.entityId;

            if (pendingResizeCommands_.size() <= idx) {
                pendingResizeCommands_.resize(idx + 1);
            }

            pendingResizeCommands_[idx] = std::move(command);
            return true;
        }

        /**
         * @brief Queues a window-close request.
         *
         * @param command Window-close command.
         *
         * @return `true` when the command was queued.
         */
        bool submit(WindowCloseCommand<THandle>&& command)  noexcept {
            pendingCloseCommands_.push_back(std::move(command));
            return true;
        }

        /**
         * @brief Marks platform shutdown for execution during the next `flush(...)`.
         *
         * @param command Shutdown marker command.
         *
         * @return `true` when the command was accepted.
         */
        bool submit(ShutdownCommand&& command)  noexcept {
            shouldShutdown_ = true;
            return true;
        }

        /**
         * @brief Registers this manager as handler for supported platform/window commands.
         *
         * @param commandHandlerRegistry Registry used for command-handler registration.
         */
        void init(CommandHandlerRegistry& commandHandlerRegistry) noexcept {

            commandHandlerRegistry.handleCommands<
                WindowCreateCommand<THandle>,
                PlatformInitCommand,
                WindowResizeCommand<THandle>,
                SwapBuffersCommand<THandle>,
                PollEventsCommand,
                WindowCloseCommand<THandle>,
                ShutdownCommand
            >(*this);
        }
    };


}

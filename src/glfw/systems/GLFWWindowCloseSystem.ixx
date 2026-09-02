/**
 * @file GLFWWindowCloseSystem.ixx
 * @brief System that converts native GLFW close requests into engine window-close commands.
 */
module;

#include <GLFW/glfw3.h>

export module helios.glfw.systems.GLFWWindowCloseSystem;



import helios.engine.runtime.gameloop.types;
import helios.ecs.entity.EntityWorld;
import helios.ecs.entity.EntityAccessSet;
import helios.ecs.entity.Query;



import helios.ecs.command.types;

import helios.glfw.components;
import helios.engine.platform.window.components;
import helios.engine.platform.window.commands.WindowCloseCommand;
import helios.ecs;

using namespace helios::engine::runtime;
using namespace helios::ecs;
using namespace helios::ecs::common::concepts;
using namespace helios::glfw::components;
using namespace helios::engine::platform::window::commands;
using namespace helios::engine::platform::window::components;
using namespace helios::ecs::components;
export namespace helios::glfw::systems {

    /**
     * @brief Emits `WindowCloseCommand` when GLFW reports a close request.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle>
    class GLFWWindowCloseSystem {

        using EntityWorld = ecs::entity::EntityWorld;

        template<typename TRead, typename TWrite>
        using Query = ecs::entity::Query<THandle, TRead, TWrite>;

        template<typename ... TReads>
        using Read = ecs::entity::ReadSet<TReads...>;

        template<typename ... TWrites>
        using Write = ecs::entity::WriteSet<TWrites...>;

    public:

        using HandleType = THandle;

        using CommandBuffer = ecs::command::TypedCommandBuffer<WindowCloseCommand<THandle>>;

        /**
         * @brief Scans shown windows and queues close commands for requested closures.
         *
         * @param query Frame-local query over shown GLFW-backed windows.
         */
        void update(
            Query<
                Read<WindowComponent<THandle>,
                    GLFWWindowHandleComponent<THandle>,
                    WindowShownComponent<THandle>
                >,
                Write<>
            > query,
            CommandBuffer& cmdBuffer
        ) noexcept {

            for (auto [entity, wc, glfw, wsc] : query.withActive()) {
                if (glfwWindowShouldClose(glfw->handle)) {
                    cmdBuffer.template add<WindowCloseCommand<THandle>>(
                        entity.handle()
                    );
                }
            }
        }

    };

}
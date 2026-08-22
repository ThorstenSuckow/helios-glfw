/**
 * @file GLFWWindowCloseSystem.ixx
 * @brief System that converts native GLFW close requests into engine window-close commands.
 */
module;

#include <GLFW/glfw3.h>

export module helios.glfw.systems.GLFWWindowCloseSystem;



import helios.engine.runtime.world.UpdateContext;
import helios.engine.runtime.world.types;
import helios.engine.runtime.concepts;


import helios.ecs.command.types;

import helios.glfw.components;
import helios.engine.platform.window.components;
import helios.engine.platform.window.commands.WindowCloseCommand;
import helios.ecs;
import helios.engine.platform.window.concepts.IsWindowHandle;

using namespace helios::engine::runtime::world;
using namespace helios::ecs;
using namespace helios::ecs::common::concepts;
using namespace helios::glfw::components;
using namespace helios::engine::platform::window::commands;
using namespace helios::engine::platform::window::components;
using namespace helios::ecs::components;
using namespace helios::engine::platform::window::concepts;
export namespace helios::glfw::systems {

    /**
     * @brief Emits `WindowCloseCommand` when GLFW reports a close request.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle>
    class GLFWWindowCloseSystem {
    public:


        using CommandBuffer = ecs::command::TypedCommandBuffer<WindowCloseCommand<THandle>>;

        /**
         * @brief Scans shown windows and queues close commands for requested closures.
         *
         * @param updateContext Frame-local update context.
         */
        void update(UpdateContext& updateContext, CommandBuffer& cmdBuffer) noexcept {

            for (auto [entity, wc, glfw, wsc]: updateContext.template view<
                THandle,
                WindowComponent<THandle>,
                GLFWWindowHandleComponent<THandle>,
                WindowShownComponent<THandle>
                >().withActive()) {
                if (glfwWindowShouldClose(glfw->handle)) {
                    cmdBuffer.template add<WindowCloseCommand<THandle>>(
                        entity.handle()
                    );
                }
            }
        }

    };

}
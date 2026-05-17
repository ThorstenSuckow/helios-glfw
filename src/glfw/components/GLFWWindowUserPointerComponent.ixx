/**
 * @file GLFWWindowUserPointerComponent.ixx
 * @brief Component storing typed user-pointer payload for GLFW callbacks.
 */
module;



export module helios.glfw.components.GLFWWindowUserPointerComponent;

import helios.engine.runtime.world.GameWorld;
import helios.glfw.types.GLFWWindowUserPointer;
import helios.engine.platform.window.concepts.IsWindowHandle;
import helios.engine.runtime.messaging.command.concepts.IsPlatformCommandBuffer;

using namespace helios::engine::runtime::world;
using namespace helios::engine::platform::window::concepts;
using namespace helios::glfw::types;
using namespace helios::engine::runtime::messaging::command::concepts;
export namespace helios::glfw::components {


    /**
     * @brief Associates a window entity with callback payload used by GLFW user-pointer API.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle, typename TPlatformCommandBuffer>
    requires IsWindowHandle<THandle> && IsPlatformCommandBuffer<TPlatformCommandBuffer>
    struct GLFWWindowUserPointerComponent {
        /** @brief Typed payload exposed to GLFW callbacks via `glfwSetWindowUserPointer`. */
        GLFWWindowUserPointer<THandle, TPlatformCommandBuffer> userPointer;

    };

} // namespace helios::glfw::components

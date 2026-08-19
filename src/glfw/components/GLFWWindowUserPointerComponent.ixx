/**
 * @file GLFWWindowUserPointerComponent.ixx
 * @brief Component storing typed user-pointer payload for GLFW callbacks.
 */
module;



export module helios.glfw.components.GLFWWindowUserPointerComponent;

import helios.engine.runtime.world.GameWorld;
import helios.glfw.types.GLFWWindowUserPointer;
import helios.engine.platform.window.concepts.IsWindowHandle;

using namespace helios::engine::runtime::world;
using namespace helios::engine::platform::window::concepts;
using namespace helios::glfw::types;
export namespace helios::glfw::components {


    /**
     * @brief Associates a window entity with callback payload used by GLFW user-pointer API.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle, typename TCommandBuffer>
    struct GLFWWindowUserPointerComponent {
        /** @brief Typed payload exposed to GLFW callbacks via `glfwSetWindowUserPointer`. */
        GLFWWindowUserPointer<THandle, TCommandBuffer> userPointer;

    };

} // namespace helios::glfw::components

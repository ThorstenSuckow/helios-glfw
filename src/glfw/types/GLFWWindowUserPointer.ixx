/**
 * @file GLFWWindowUserPointer.ixx
 * @brief Typed payload passed through GLFW's opaque user-pointer mechanism.
 */
module;

export module helios.glfw.types.GLFWWindowUserPointer;

import helios.engine.platform.window.concepts.IsWindowHandle;
import helios.ecs.common.concepts;


using namespace helios::engine::platform::window::concepts;
using namespace helios::ecs::common::concepts;
export namespace helios::glfw::types {

    /**
     * @brief Callback payload allowing GLFW callbacks to resolve window entity and game world.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle, typename TPlatformCommandBuffer>
    requires IsWindowHandle<THandle>
    struct GLFWWindowUserPointer {
        /** @brief Window entity handle associated with the native window. */
        THandle windowHandle;

        /** @brief Runtime platform world used by callbacks to enqueue or mutate state. */
        TPlatformCommandBuffer* platformCommandBuffer = nullptr;
    };

}

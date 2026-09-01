/**
 * @file GLFWWindowHandleComponent.ixx
 * @brief Component storing native GLFW window handles for window entities.
 */
module;


#include <GLFW/glfw3.h>

export module helios.glfw.components.GLFWWindowHandleComponent;

export namespace helios::glfw::components {

    /**
     * @brief Binds a window entity to its native `GLFWwindow*` handle.
     *
     * @tparam THandle Window handle type.
     */
    template<typename THandle>
    struct GLFWWindowHandleComponent {


        using HandleType = THandle;

        /** @brief Native GLFW window pointer. */
        GLFWwindow* handle = nullptr;

    };

} // namespace helios::glfw::components

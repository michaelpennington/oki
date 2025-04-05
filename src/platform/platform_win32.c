#if defined(KBUILD_WINDOWS)
#include "platform/platform.h"

#include "containers/darray.h"
#include "core/input.h"
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

typedef struct platform_state {
  HINSTANCE h_instance;
  HWND hwnd;
  VkSurfaceKHR vk_surface;

  f64 clock_frequency;
  LARGE_INTEGER start_time;
} platform_state;

static platform_state *state_ptr;

static bool enable_colors();

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param,
                                       LPARAM l_param);

#define WINDOW_CLASS "kohi_window_class"

bool platform_startup(void **plat_state, platform_config config) {
  *plat_state = platform_allocate(sizeof(platform_state), false);
  platform_zero_memory(*plat_state, sizeof(platform_state));
  state_ptr = *plat_state;

  state_ptr->h_instance = GetModuleHandleA(nullptr);

  HICON icon = LoadIcon(state_ptr->h_instance, IDI_APPLICATION);
  WNDCLASSA wc;
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = win32_process_message;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = state_ptr->h_instance;
  wc.hIcon = icon;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.lpszClassName = WINDOW_CLASS;

  if (!RegisterClassA(&wc)) {
    MessageBoxA(nullptr, "Window registration failed", "Error",
                MB_ICONEXCLAMATION | MB_OK);
    return false;
  }

  if (!enable_colors()) {
    return false;
  }

  u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX |
                     WS_MINIMIZEBOX | WS_THICKFRAME;
  u32 window_ex_style = WS_EX_APPWINDOW;

  RECT border_rect = {0, 0, 0, 0};
  AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

  u32 window_x = config.x + border_rect.left;
  u32 window_y = config.y + border_rect.top;
  u32 window_width = config.width + border_rect.right - border_rect.left;
  u32 window_height = config.height + border_rect.bottom - border_rect.top;

  HWND handle = CreateWindowExA(
      window_ex_style, WINDOW_CLASS, config.application_name, window_style,
      (u16)window_x, (u16)window_y, (u16)window_width, (u16)window_height,
      nullptr, nullptr, state_ptr->h_instance, nullptr);

  if (!handle) {
    MessageBoxA(nullptr, "Window creation failed", "Error",
                MB_ICONEXCLAMATION | MB_OK);
    return false;
  }
  state_ptr->hwnd = handle;

  u32 should_activate = 1;
  i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
  ShowWindow(state_ptr->hwnd, show_window_command_flags);

  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  state_ptr->clock_frequency = 1 / (f64)frequency.QuadPart;
  QueryPerformanceCounter(&state_ptr->start_time);

  return true;
}

void platform_shutdown() {
  if (state_ptr) {
    if (state_ptr->hwnd) {
      DestroyWindow(state_ptr->hwnd);
      state_ptr->hwnd = nullptr;
    }
    free(state_ptr);
    state_ptr = nullptr;
  }
}

bool platform_pump_messages() {
  MSG message;
  while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  return true;
}

void *platform_allocate(u64 size, bool aligned) {
  (void)aligned;
  return malloc(size);
}
void platform_free(void *block, bool aligned) {
  (void)aligned;
  free(block);
}
void *platform_zero_memory(void *block, u64 size) {
  return memset(block, 0, size);
}
void *platform_copy_memory(void *dest, const void *source, u64 size) {
  return memcpy(dest, source, size);
}
void *platform_set_memory(void *dest, i32 value, u64 size) {
  return memset(dest, value, size);
}

f64 platform_get_absolute_time() {
  if (state_ptr) {
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * state_ptr->clock_frequency;
  }
  return 0.0;
}

void platform_sleep(u32 ms) { Sleep(ms); }

void platform_get_required_extension_names(const char ***names) {
  darray_push(names, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param,
                                       LPARAM l_param) {
  switch (msg) {
  case WM_ERASEBKGND:
    // Notify the OS that erasing will be handled by the application to prevent
    // flicker
    return 1;
  case WM_CLOSE:
    // event_context data = {0};
    // event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_SIZE: {
    RECT rec;
    GetClientRect(hwnd, &rec);
    u32 width = rec.right - rec.left;
    u32 height = rec.bottom - rec.top;
    (void)width;
    (void)height;
  } break;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP: {
    bool pressed = (bool)(msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    keys key = (u16)w_param;
    input_process_key(key, pressed);
  } break;
  case WM_MOUSEMOVE: {
    // Mouse move
    i32 x_position = GET_X_LPARAM(l_param);
    i32 y_position = GET_Y_LPARAM(l_param);
    // TODO: Input processing
    input_process_mouse_move((i16)x_position, (i16)y_position);
  } break;
  case WM_MOUSEWHEEL: {
    i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
    if (z_delta != 0) {
      // Flatten input to OS-independent {-1, 1}
      z_delta = (z_delta < 0) ? -1 : 1;
      input_process_mouse_wheel((i8)z_delta);
    }
  } break;
  case WM_LBUTTONDOWN: {
    input_process_button(BUTTON_LEFT, true);
  } break;
  case WM_MBUTTONDOWN: {
    input_process_button(BUTTON_MIDDLE, true);
  } break;
  case WM_RBUTTONDOWN: {
    input_process_button(BUTTON_RIGHT, true);
  } break;
  case WM_LBUTTONUP: {
    input_process_button(BUTTON_LEFT, false);
  } break;
  case WM_MBUTTONUP: {
    input_process_button(BUTTON_MIDDLE, false);
  } break;
  case WM_RBUTTONUP: {
    input_process_button(BUTTON_RIGHT, false);
  } break;
  default:
  }

  return DefWindowProcA(hwnd, msg, w_param, l_param);
}

static bool enable_colors(void) {
  HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h_out == INVALID_HANDLE_VALUE) {
    return false;
  }
  HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);
  if (h_in == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD original_out_mode = 0;
  DWORD original_in_mode = 0;

  if (!GetConsoleMode(h_out, &original_out_mode)) {
    return false;
  }
  if (!GetConsoleMode(h_in, &original_in_mode)) {
    return false;
  }

  DWORD requested_out_modes =
      ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
  DWORD requested_in_modes = ENABLE_VIRTUAL_TERMINAL_INPUT;

  DWORD out_mode = original_out_mode | requested_out_modes;
  if (!SetConsoleMode(h_out, out_mode)) {
    requested_out_modes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    out_mode = original_out_mode | requested_out_modes;
    if (!SetConsoleMode(h_out, out_mode)) {
      return false;
    }
  }

  DWORD in_mode = original_in_mode | requested_in_modes;
  return (bool)(SetConsoleMode(h_in, in_mode));
}

bool platform_create_vulkan_surface(VkInstance instance,
                                    VkAllocationCallbacks *allocator,
                                    VkSurfaceKHR *surface) {
  VkWin32SurfaceCreateInfoKHR create_info = {
      .stype = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hinstance = state_ptr->h_instance,
      .hwnd = state_ptr->hwnd,
  };
  VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info,
                                            context->allocator, surface);
  if (result != VK_SUCCESS) {
    kfatal("Vulkan surface creation failed.");
    return false;
  }

  state_ptr->surface = *surface;
  return true;
}

#endif

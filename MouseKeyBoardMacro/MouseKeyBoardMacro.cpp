#include <iostream>
#include <string>
#include <array>
#include <bit>
#include <vector>
#include <queue>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN        
#include <windows.h>


void Print() {
    std::cout << std::endl;
}


template<typename T, typename ...TS>
void Print(T value, TS ...ts) {
    std::cout << value << "   ";
    Print(ts...);
}


std::string GetWin32ErrorMessage(DWORD errorCode) {

	char buffer[4096];

	auto length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, 0, buffer, sizeof(buffer), nullptr);

	return std::string{ buffer, length };
}

void Exit(std::string message) {
	int errorCode = GetLastError();
	std::cout << message << "	" << GetWin32ErrorMessage(errorCode) << std::endl;
    exit(errorCode);
}


class CreateWindowHandle {

    static void _CreateWindowClass(HINSTANCE moduleHandle, LPCWSTR windowsClassName) {
	    WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = DefWindowProcW;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = moduleHandle;
        wcex.hIcon          = nullptr;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName   = nullptr;
        wcex.lpszClassName  = windowsClassName;
        wcex.hIconSm        = nullptr;

        if (FALSE == RegisterClassExW(&wcex)) {
            Exit("create window class error");
        }
    }

    static auto _CreateWindow(HINSTANCE moduleHandle, LPCWSTR windowsClassName) {
       
        auto windowsHandle = CreateWindowExW(0L, windowsClassName, L"Window", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, moduleHandle, nullptr);

        if (windowsHandle == FALSE) {
            Exit("create window error");
        }

        return windowsHandle;
    }

    HWND m_windowHandle;

public:
    CreateWindowHandle() {
        auto moduleHandle = static_cast<HINSTANCE>(GetModuleHandleW(nullptr));
      
        WCHAR windowsClassName[] = L"fsdfsrewrwegfdgfd";

        CreateWindowHandle::_CreateWindowClass(moduleHandle, windowsClassName);

        m_windowHandle = CreateWindowHandle::_CreateWindow(moduleHandle, windowsClassName);
    }

    auto GetHandle() {
        return m_windowHandle;
    }
};

void CreateMouseRawInput(HWND handle) {

    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = handle;
    
    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE) {
        Exit("create mouse raw input error");
    }
}

void CreateKeyboardRawInput(HWND handle) {

    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = RIDEV_NOLEGACY | RIDEV_INPUTSINK;
    rid.hwndTarget = handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE) {
        Exit("create keyboard raw input error");
    }
}

enum class InputFlag : unsigned char
{
    Down = 0x00,
  
    Up = KEYEVENTF_KEYUP
};


enum class VKCode : unsigned char {

    MouseWheel = 0x00,

    MouseLeft = 0x01,

    MouseRight = 0x02,

    MouseMiddle = 0x04,

    H = 0x48,

    C = 0x43,

    R = 0x52,

    G = 0x47,

    RightShift = 0x10,

    ArrayUp=0x26,
    ArrayDown = 40,
    ArrayLeft = 37,
    ArrayRight = 39,

    P=0x50


};


class Input {

    static_assert(
        std::endian::native == std::endian::little &&
        std::is_same_v<std::underlying_type_t<InputFlag>, unsigned char> &&
        std::is_same_v<std::underlying_type_t<VKCode>, unsigned char>, "error");
    using NT = uint16_t;

    constexpr static NT OFFSET = sizeof(NT) * 8 / 2;

    NT m_value;

public:
    Input() :m_value(0) {}


    Input(InputFlag flag, VKCode code) {

        auto value = static_cast<NT>(flag);

        value <<= OFFSET;

        value |= static_cast<NT>(code);

        m_value = value;

    }

    NT GetValue() {
        return m_value;
    }
};

bool operator!=(Input left, Input right) {
    return left.GetValue() != right.GetValue();
}



template<typename T, size_t SIZE>
requires(std::is_default_constructible_v<T> && std::is_trivially_copy_assignable_v<T> && SIZE != 0 && (SIZE_MAX % SIZE == SIZE - 1))
class fixd {


    static size_t GetIndex(size_t index) {
        return index % SIZE;
    }


    class reverse_iterator {
        std::array<T, SIZE>& m_array;
        size_t m_index;


    public:
        reverse_iterator(std::array<T, SIZE>& array, size_t index) : m_array(array), m_index(index) {}


        reverse_iterator& operator++() {
            m_index--;

            return *this;
        }

        bool operator!=(const reverse_iterator& right) {
            return GetIndex(m_index) != GetIndex(right.m_index);
        }

        T& operator*() {
            return m_array[GetIndex(m_index)];
        }

    };

    std::array<T, SIZE> m_array;

    size_t m_index;
public:
    fixd() :m_array(), m_index(0) {

    }

    void Add(const T& value) {

        m_array[GetIndex(m_index)] = value;

        m_index++;
    }

    auto rbegin() {
        return fixd<T, SIZE>::reverse_iterator{ m_array, m_index - 1 };
    }

    auto rend() {
        return fixd<T, SIZE>::reverse_iterator{ m_array, m_index };
    }
};



class LinkMap {

    constexpr static size_t SIZE = 32;

    class Node {
        std::vector<Input> m_key;

        std::vector<INPUT> m_value;

    public:
        Node(const std::vector<Input>& key, const std::vector<INPUT>& value) : m_key(key), m_value(value) {}
    


        auto& GetKey() {
            return m_key;
        }

        auto& GetValue() {
            return m_value;
        }

    };


    std::vector<Node> m_nodes;

    fixd<Input, SIZE> m_keys;


    static bool Get(fixd<Input, SIZE>& left, const std::vector<Input>& right) {

        auto left_start = left.rbegin();

        auto left_end = left.rend();

        auto right_start = right.rbegin();

        auto right_end = right.rend();


        while (left_start != left_end && right_start!=right_end)
        {

            if (*left_start != *right_start) {
                return false;
            }



            ++left_start;
            ++right_start;
        }

        return true;
    }

public:
    LinkMap() : m_nodes(), m_keys() {
      
    }

    void Add(const std::vector<Input>& key, const std::vector<INPUT>& value) {
        m_nodes.emplace_back(key, value);
    }

    std::vector<INPUT>* Get(Input key) {

        m_keys.Add(key);


        for (auto& node : m_nodes) {
         
            if (Get(m_keys, node.GetKey())) {
                return &node.GetValue();
            }
        }

        return nullptr;
    }
};

auto GetScanCode(VKCode code) {
    auto value = MapVirtualKeyW(static_cast<UINT>(code), MAPVK_VK_TO_VSC);

    if (value == 0) {
        Exit("get scan code error");
    }
    else {
        return value;
    }
}



INPUT CreateKeyBoardInput(InputFlag flag, VKCode code) {


    KEYBDINPUT keyInput = {};

    keyInput.wVk = static_cast<WORD>(code);
   
    keyInput.wScan = static_cast<WORD>(GetScanCode(code));

    keyInput.dwFlags = static_cast<DWORD>(flag);


    keyInput.time = 0;
    
    keyInput.dwExtraInfo = 0;
   

    INPUT input = {};

    input.type = INPUT_KEYBOARD;

    input.ki = keyInput;

    return input;

}


class Info {

public:
    static auto& GetMouseData() {
        static LinkMap data{};
        return data;
    }

    static auto& GetKeyBoardData() {
        static LinkMap data{};

        return data;
    }

};


void SendMacro(std::vector<INPUT>* item) {
    if (item != nullptr) {
       
        SendInput(static_cast<UINT>(item->size()), item->data(), sizeof(INPUT));
    }
}

void MouseMacro(Input input) {

   decltype(auto) data = Info::GetMouseData();

   SendMacro(data.Get(input));
}


void KeyBoardMacro(Input input) {

    decltype(auto) data = Info::GetKeyBoardData();

    SendMacro(data.Get(input));
}


void KeyboardRawInput(RAWKEYBOARD& data) {
   
    if ((data.Flags & RI_KEY_BREAK) == RI_KEY_MAKE) {
       
        KeyBoardMacro(Input{ InputFlag::Down, static_cast<VKCode>(data.VKey) });

    }
    else if ((data.Flags & RI_KEY_BREAK) == RI_KEY_BREAK) {
      
        KeyBoardMacro(Input{ InputFlag::Up, static_cast<VKCode>(data.VKey) });

    }
    else {
        
    }
}


void MouseRawInput(RAWMOUSE& data) {
    
    if (data.usButtonFlags == 0) {

    }
    else if (data.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN) {
       
        MouseMacro(Input{ InputFlag::Down, VKCode::MouseLeft });
    }
    else if (data.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP) {
     
        MouseMacro(Input{ InputFlag::Up, VKCode::MouseLeft });
    }
    else if (data.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN) {


        MouseMacro(Input{ InputFlag::Down, VKCode::MouseMiddle });
    }
    else if (data.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP) {

        MouseMacro(Input{ InputFlag::Up, VKCode::MouseMiddle });
    }
    else if (data.usButtonFlags == RI_MOUSE_WHEEL) {

        if (data.usButtonData == 120) {

            MouseMacro(Input{ InputFlag::Up, VKCode::MouseWheel });
        }
        else {

            MouseMacro(Input{ InputFlag::Down, VKCode::MouseWheel });
        }
    }
    else if (data.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN) {
      
        MouseMacro(Input{ InputFlag::Down, VKCode::MouseRight });
    }
    else if (data.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP) {
       
        MouseMacro(Input{ InputFlag::Up, VKCode::MouseRight });
    }
    
}

template<UINT SIZE>
void frowRawInput(std::array<char, SIZE>& buffer, LPARAM lParam) {

    UINT dwSize = SIZE;

    if (-1 == GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER))) {
        Exit("get raw input data error");
    }
   
    auto raw = reinterpret_cast<RAWINPUT*>(buffer.data());

    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        KeyboardRawInput(raw->data.keyboard);
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        MouseRawInput(raw->data.mouse);
    }
    else {
        Exit("other raw input message");
    }
}

void AddMouseData(std::vector<Input> key, std::vector<INPUT> value) {
    Info::GetMouseData().Add(key, value);
}

void AddKeyBoardData(std::vector<Input> key, std::vector<INPUT> value) {
    Info::GetKeyBoardData().Add(key, value);
}

int Start() {
    CreateWindowHandle window{};


    CreateMouseRawInput(window.GetHandle());

    CreateKeyboardRawInput(window.GetHandle());


    std::array<char, 1024> buffer{};

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_INPUT) {

            frowRawInput(buffer, msg.lParam);

            if (GET_RAWINPUT_CODE_WPARAM(msg.wParam) == RIM_INPUT) {
                DispatchMessage(&msg);
            }
            else {

            }

        }
        else {
            DispatchMessage(&msg);
        }   
    }

    return (int)msg.wParam;
}


int main() {
    
    AddMouseData({
        Input{InputFlag::Down, VKCode::MouseMiddle},
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::H),
            CreateKeyBoardInput(InputFlag::Up, VKCode::H),
            
        });

    AddMouseData({
       Input{InputFlag::Up, VKCode::MouseMiddle},
        },
        {
           CreateKeyBoardInput(InputFlag::Down, VKCode::C),
            CreateKeyBoardInput(InputFlag::Up, VKCode::C),
        });

    
    AddMouseData({
        Input{InputFlag::Down, VKCode::MouseMiddle},
        Input{InputFlag::Down, VKCode::MouseLeft},
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::R),
            CreateKeyBoardInput(InputFlag::Up, VKCode::R),
        });

    AddKeyBoardData({

        Input{InputFlag::Down, VKCode::ArrayUp},
        Input{InputFlag::Up, VKCode::ArrayUp},
        Input{InputFlag::Down, VKCode::ArrayUp},
       
        },
        {
            CreateKeyBoardInput(InputFlag::Down, VKCode::P),
        });

    AddKeyBoardData({
        
       Input{InputFlag::Down, VKCode::ArrayUp},
       Input{InputFlag::Down, VKCode::ArrayUp},
       Input{InputFlag::Up, VKCode::ArrayUp},

        },
        {
            CreateKeyBoardInput(InputFlag::Up, VKCode::P),   
        });


    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayDown},
      Input{InputFlag::Up, VKCode::ArrayDown},

        },
        {
            CreateKeyBoardInput(InputFlag::Up, VKCode::P),
        });

    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayLeft},
      Input{InputFlag::Up, VKCode::ArrayLeft},

        },
        {
            CreateKeyBoardInput(InputFlag::Up, VKCode::P),
        });

    AddKeyBoardData({

      Input{InputFlag::Down, VKCode::ArrayRight},
      Input{InputFlag::Up, VKCode::ArrayRight},

        },
        {
            CreateKeyBoardInput(InputFlag::Up, VKCode::P),
        });
    return Start();
}
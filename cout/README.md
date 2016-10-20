# uwp-cpp-examples
Using std::cout and std::cwout in a Windows 10 UWP app

In order to override std::cout to use OutputDebugString in a C++ UWP app, do the following:

1. Add #include "OutputDebugStringBuf.h" to App.xaml.h

1. Add to App.xaml.h

```c++
#ifdef _DEBUG
        // overrides std::cout and std::wcout to use DebugOutputString
        OutputDebugStringBufA m_charDebugOutput;
        OutputDebugStringBufW m_wcharDebugOutput;
#endif
```

1. Use std::cout and std::wcout normally in the app. 

 ```c++
   std::cout << "***App.xaml.cpp: Hello world!***" << std::endl;
    std::wcout << L"***App.xaml.cpp: Hello wide world!***" << std::endl;
```

For example using the above code in  App.xaml.cpp will produce the following output in the Visual Studio Debug output window:
 ```c++
***App.xaml.cpp: Hello world!***
***App.xaml.cpp: Hello wide world!***
```

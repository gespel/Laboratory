# Set C++20 and other defaults for SharedCode target
target_compile_features(SharedCode INTERFACE cxx_std_20)

# Enable fast math on non-MSVC compilers
if (NOT MSVC)
    target_compile_options(SharedCode INTERFACE -ffast-math)
endif()

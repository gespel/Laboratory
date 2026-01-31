# Intel IPP Support
# This is optional and provides performance optimizations

if(EXISTS "/opt/intel/oneapi/ipp/latest")
    message(STATUS "Intel IPP found, enabling IPP support")
    set(ENV{IPPROOT} "/opt/intel/oneapi/ipp/latest")
    # IPP will be automatically detected by JUCE
else()
    message(STATUS "Intel IPP not found, continuing without IPP support")
endif()

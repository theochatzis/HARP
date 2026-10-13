harp_add_executable(harpRun SOURCES harpRun.cc DEPENDS HarpDataFormats)
harp_add_executable(harpPlot SOURCES harpPlot.cc DEPENDS HarpPlot HarpFit)
harp_add_executable(harpInspect SOURCES harpInspect.cc)

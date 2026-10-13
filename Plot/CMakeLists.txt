harp_add_library(Plot
  SOURCES src/PlotStyle.cc src/TdrStyle.cc src/HistLoader.cc src/Plotter.cc src/PlotterFactory.cc
  DEPENDS HarpCore HarpFit)

harp_add_plugin(Plotters
  SOURCES plugins/Overlay1D.cc plugins/ResponseVsEnergy.cc plugins/Profile2D.cc
  DEPENDS HarpPlot HarpFit ROOT::HistPainter)

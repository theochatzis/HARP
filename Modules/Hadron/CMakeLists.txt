harp_add_plugin(Hadron
  SOURCES
    plugins/HadronTruthMatcher.cc
    plugins/ShowerVariablesProducer.cc
    plugins/ResponseAnalyzer.cc
    plugins/PIDAnalyzer.cc
    plugins/MatchingQAAnalyzer.cc
    plugins/EventSummaryAnalyzer.cc
    plugins/EnergyBudgetAnalyzer.cc
  DEPENDS HarpDataFormats)

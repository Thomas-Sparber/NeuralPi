#add_subdirectory(headless)

target_sources(NeuralPi PRIVATE
	CabSim.cpp
	Eq4Band.cpp
	PluginProcessor.cpp
	NeuralNetwork.cpp
)

#target_precompile_headers(NeuralPi PRIVATE pch.h)

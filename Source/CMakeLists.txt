#add_subdirectory(headless)

target_sources(NeuralPi PRIVATE
	AmpOSCReceiver.h
	Delay.h
	Eq4Band.cpp
	Eq4Band.h
	PluginProcessor.cpp
	PluginProcessor.h
	RTNeuralLSTM.cpp
	RTNeuralLSTM.h
)

#target_precompile_headers(NeuralPi PRIVATE pch.h)

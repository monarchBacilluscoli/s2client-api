#include"rolling_ea.h"

namespace sc2{
    void RollingEA::Initialize(const ObservationInterface* observation){
        InitializeFromObservation(observation);
    }

    void RollingEA::InitializeFromObservation(const ObservationInterface* observation){
        
    }

    void RollingEA::ShowGraphEachGeneration(){
        EvolutionaryAlgorithm<Command>::ShowGraphEachGeneration(); //overall status
        //todo Here I just need to show the scatter graph
        
    }
}
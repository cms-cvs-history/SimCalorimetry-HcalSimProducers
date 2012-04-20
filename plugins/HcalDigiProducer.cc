#include "SimCalorimetry/HcalSimProducers/plugins/HcalDigiProducer.h"
#include "FWCore/Framework/interface/EDProducer.h"

HcalDigiProducer::HcalDigiProducer(edm::ParameterSet const& pset, edm::EDProducer& mixMod) :
    DigiAccumulatorMixMod(),
    theDigitizer_(pset) {
    std::string const instance("simHcalUnsuppressedDigis");
    mixMod.produces<HBHEDigiCollection>(instance);
    mixMod.produces<HODigiCollection>(instance);
    mixMod.produces<HFDigiCollection>(instance);
    mixMod.produces<ZDCDigiCollection>(instance);
}

void
HcalDigiProducer::initializeEvent(edm::Event const& event, edm::EventSetup const& es) {
  theDigitizer_.initializeEvent(event, es);
}

void
HcalDigiProducer::finalizeEvent(edm::Event& event, edm::EventSetup const& es) {
  theDigitizer_.finalizeEvent(event, es);
}

void
HcalDigiProducer::accumulate(edm::Event const& event, edm::EventSetup const& es) {
  theDigitizer_.accumulate(event, es);
}

void
HcalDigiProducer::accumulate(PileUpEventPrincipal const& event, edm::EventSetup const& es) {
  theDigitizer_.accumulate(event, es);
}

void
HcalDigiProducer::beginRun(edm::Run&, edm::EventSetup const& es) {
  theDigitizer_.beginRun(es);
}

void
HcalDigiProducer::endRun(edm::Run&, edm::EventSetup const&) {
  theDigitizer_.endRun();
}

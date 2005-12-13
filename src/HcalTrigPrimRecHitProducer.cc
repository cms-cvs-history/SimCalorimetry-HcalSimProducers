using namespace std;
#include "SimCalorimetry/HcalSimProducers/src/HcalTrigPrimRecHitProducer.h"
#include "FWCore/EDProduct/interface/EDProduct.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "FWCore/Framework/interface/MakerMacros.h"

using namespace cms;


HcalTrigPrimRecHitProducer::HcalTrigPrimRecHitProducer(const edm::ParameterSet& ps)
: theAlgo()
{
  produces<HcalTrigPrimRecHitCollection>();
}


void HcalTrigPrimRecHitProducer::produce(edm::Event& e, const edm::EventSetup& eventSetup) {

  edm::Handle<HBHEDigiCollection> hbheDigis;
  edm::Handle<HFDigiCollection>   hfDigis;

  // TODO make labels for unsuppressed
  e.getByType(hbheDigis);
  e.getByType(hfDigis);



  // Step B: Create empty output

  auto_ptr<HcalTrigPrimRecHitCollection> result(new HcalTrigPrimRecHitCollection());

  // Step C: Invoke the algorithm, passing in inputs and getting back outputs.
  theAlgo.run(*hbheDigis,  *hfDigis, *result);

  // Step D: Put outputs into event
  e.put(result);

}



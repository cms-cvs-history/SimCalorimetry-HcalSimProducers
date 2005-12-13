using namespace std;
#include "SimCalorimetry/HcalSimProducers/src/HcalDigiProducer.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "SimCalorimetry/CaloSimAlgos/interface/CaloTDigitizer.h"
#include "SimCalorimetry/CaloSimAlgos/interface/CaloShapeIntegrator.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "CalibFormats/HcalObjects/interface/HcalDbService.h"
#include "CalibFormats/HcalObjects/interface/HcalDbRecord.h"


HcalDigiProducer::HcalDigiProducer(const edm::ParameterSet& ps) {

  produces<HBHEDigiCollection>();
  produces<HODigiCollection>();
  produces<HFDigiCollection>();

  theParameterMap = new HcalSimParameterMap();
  theHcalShape = new HcalShape();
  theHFShape = new HFShape();
  theHcalIntegratedShape = new CaloShapeIntegrator(theHcalShape);
  theHFIntegratedShape = new CaloShapeIntegrator(theHFShape);



  theHcalResponse = new CaloHitResponse(theParameterMap, theHcalIntegratedShape);
  theHFResponse = new CaloHitResponse(theParameterMap, theHFIntegratedShape);


  theNoisifier = new HcalNoisifier();
  theCoder = new HcalNominalCoder();
  theElectronicsSim = new HcalElectronicsSim(theNoisifier, theCoder);

  theHBHEDigitizer = new HBHEDigitizer(theHcalResponse, theElectronicsSim);
  theHODigitizer = new HODigitizer(theHcalResponse, theElectronicsSim);
  theHFDigitizer = new HFDigitizer(theHFResponse, theElectronicsSim);

}


HcalDigiProducer::~HcalDigiProducer() {
  delete theParameterMap;
  delete theHcalShape;
  delete theHFShape;
  delete theHcalIntegratedShape;
  delete theHFIntegratedShape;
  delete theHcalResponse;
  delete theHFResponse;
  delete theElectronicsSim;
  delete theNoisifier;
  delete theCoder;
}


void HcalDigiProducer::produce(edm::Event& e, const edm::EventSetup& eventSetup) {
  // get the appropriate gains, noises, & widths for this event
  edm::ESHandle<HcalDbService> conditions;
  eventSetup.get<HcalDbRecord>().get(conditions);
  theNoisifier->setDbService(conditions.product());

  // get the correct geometry
  checkGeometry(eventSetup);

  theHBHEHits.clear();
  theHOHits.clear();
  theHFHits.clear();
  // Step A: Get Inputs

  edm::Handle<edm::PCaloHitContainer> hcalHits;
  e.getByLabel("r", "HcalHits", hcalHits);
  sortHits(*hcalHits);
//  fillFakeHits();


  // Step B: Create empty output

  auto_ptr<HBHEDigiCollection> hbheResult(new HBHEDigiCollection());
  auto_ptr<HODigiCollection> hoResult(new HODigiCollection());
  auto_ptr<HFDigiCollection> hfResult(new HFDigiCollection());


  // Step C: Invoke the algorithm, passing in inputs and getting back outputs.
  theHBHEDigitizer->run(theHBHEHits, *hbheResult);
  theHODigitizer->run(theHOHits, *hoResult);
  theHFDigitizer->run(theHFHits, *hfResult);


  // Step D: Put outputs into event
  e.put(hbheResult);
  e.put(hoResult);
  e.put(hfResult);

}


void HcalDigiProducer::sortHits(const edm::PCaloHitContainer & hits)
{
  for(edm::PCaloHitContainer::const_iterator hitItr = hits.begin();
      hitItr != hits.end(); ++hitItr)
  {
    HcalSubdetector subdet = HcalDetId(hitItr->id()).subdet();
    if(subdet == HcalBarrel || subdet == HcalEndcap) {
      theHBHEHits.push_back(*hitItr);
    }
    else if(subdet == HcalForward) {
      theHFHits.push_back(*hitItr);
    }
    else if(subdet == HcalOuter) {
      theHOHits.push_back(*hitItr);
    }
    else {
      std::cerr << "ERROR : bad HcalHit subdetector " << std::endl;
    }
  }
}

void HcalDigiProducer::fillFakeHits() {
  HcalDetId barrelDetId(HcalBarrel, 1, 1, 1);
  PCaloHit barrelHit(barrelDetId.rawId(), 0.855, 0., 0);

  HcalDetId endcapDetId(HcalEndcap, 17, 1, 1);
  PCaloHit endcapHit(endcapDetId.rawId(), 0.9, 0., 0);

  HcalDetId outerDetId(HcalOuter, 1, 1, 4);
  PCaloHit outerHit(outerDetId.rawId(), 0.45, 0., 0);

  HcalDetId forwardDetId1(HcalForward, 30, 1, 1);
  PCaloHit forwardHit1(forwardDetId1.rawId(), 35., 0., 0);

  HcalDetId forwardDetId2(HcalForward, 30, 1, 2);
  PCaloHit forwardHit2(forwardDetId2.rawId(), 48., 0., 0);

  theHBHEHits.push_back(barrelHit);
  theHBHEHits.push_back(endcapHit);
  theHOHits.push_back(outerHit);
  theHFHits.push_back(forwardHit1);
  theHFHits.push_back(forwardHit2);
}


void HcalDigiProducer::checkGeometry(const edm::EventSetup & eventSetup) {
  // TODO find a way to avoid doing this every event
  edm::ESHandle<CaloGeometry> geometry;
  eventSetup.get<IdealGeometryRecord>().get(geometry);

  vector<DetId> hbCells =  geometry->getValidDetIds(DetId::Hcal, HcalBarrel);
  vector<DetId> heCells =  geometry->getValidDetIds(DetId::Hcal, HcalEndcap);
  vector<DetId> hoCells =  geometry->getValidDetIds(DetId::Hcal, HcalOuter);
  vector<DetId> hfCells =  geometry->getValidDetIds(DetId::Hcal, HcalForward);

  // combine HB & HE
  vector<DetId> hbheCells = hbCells;
  hbheCells.insert(hbheCells.end(), heCells.begin(), heCells.end());

  theHBHEDigitizer->setDetIds(hbheCells);
  theHODigitizer->setDetIds(hoCells);
  theHFDigitizer->setDetIds(hfCells);
}



using namespace std;
#include "SimCalorimetry/HcalSimProducers/src/HcalDigiProducer.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "SimCalorimetry/CaloSimAlgos/interface/CaloTDigitizer.h"
#include "SimCalorimetry/CaloSimAlgos/interface/CaloShapeIntegrator.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "CalibFormats/HcalObjects/interface/HcalDbService.h"
#include "CalibFormats/HcalObjects/interface/HcalDbRecord.h"
#include "SimDataFormats/CrossingFrame/interface/CrossingFrame.h"
#include "SimDataFormats/CrossingFrame/interface/MixCollection.h"



HcalDigiProducer::HcalDigiProducer(const edm::ParameterSet& ps) 
: theParameterMap(new HcalSimParameterMap()),
  theHcalShape(new HcalShape()),
  theHFShape(new HFShape()),
  theHcalIntegratedShape(new CaloShapeIntegrator(theHcalShape)),
  theHFIntegratedShape(new CaloShapeIntegrator(theHFShape)),
  theHBHEResponse(new CaloHitResponse(theParameterMap, theHcalIntegratedShape)),
  theHOResponse(new CaloHitResponse(theParameterMap, theHcalIntegratedShape)),   
  theHFResponse(new CaloHitResponse(theParameterMap, theHFIntegratedShape)),
  theAmplifier(0),
  theCoderFactory(0),
  theElectronicsSim(0),
  theHitCorrection(0),
  theHBHEDigitizer(0),
  theHODigitizer(0),
  theHFDigitizer(0),
  theHBHEHits(),
  theHOHits(),
  theHFHits()
{
  
  produces<HBHEDigiCollection>();
  produces<HODigiCollection>();
  produces<HFDigiCollection>();


  theHBHEResponse->setHitFilter(&theHBHEHitFilter);
  theHOResponse->setHitFilter(&theHOHitFilter);
  theHFResponse->setHitFilter(&theHFHitFilter);

  bool doTimeSlew = ps.getParameter<bool>("doTimeSlew");
  if(doTimeSlew) {
    // no time slewing for HF
    theHitCorrection = new HcalHitCorrection(theParameterMap);
    theHBHEResponse->setHitCorrection(theHitCorrection);
    theHOResponse->setHitCorrection(theHitCorrection);
  }

  bool doNoise = ps.getParameter<bool>("doNoise");
  theAmplifier = new HcalAmplifier(theParameterMap, doNoise);
  theCoderFactory = new HcalCoderFactory(HcalCoderFactory::DB);
  theElectronicsSim = new HcalElectronicsSim(theAmplifier, theCoderFactory);

  theHBHEDigitizer = new HBHEDigitizer(theHBHEResponse, theElectronicsSim, doNoise);
  theHODigitizer = new HODigitizer(theHOResponse, theElectronicsSim, doNoise);
  theHFDigitizer = new HFDigitizer(theHFResponse, theElectronicsSim, doNoise);

}


HcalDigiProducer::~HcalDigiProducer() {
  delete theHBHEDigitizer;
  delete theHODigitizer;
  delete theHFDigitizer;
  delete theParameterMap;
  delete theHcalShape;
  delete theHFShape;
  delete theHcalIntegratedShape;
  delete theHFIntegratedShape;
  delete theHBHEResponse;
  delete theHOResponse;
  delete theHFResponse;
  delete theElectronicsSim;
  delete theAmplifier;
  delete theCoderFactory;
  delete theHitCorrection;
}


void HcalDigiProducer::produce(edm::Event& e, const edm::EventSetup& eventSetup) {
  // get the appropriate gains, noises, & widths for this event
  edm::ESHandle<HcalDbService> conditions;
  eventSetup.get<HcalDbRecord>().get(conditions);
  theAmplifier->setDbService(conditions.product());
  theCoderFactory->setDbService(conditions.product());

  // get the correct geometry
  checkGeometry(eventSetup);

  theHBHEHits.clear();
  theHOHits.clear();
  theHFHits.clear();
  // Step A: Get Inputs
  edm::Handle<CrossingFrame> cf;
  e.getByType(cf);

  // test access to SimHits
  const std::string subdet("HcalHits");
  std::auto_ptr<MixCollection<PCaloHit> > col(new MixCollection<PCaloHit>(cf.product(), subdet));
  //fillFakeHits();

  if(theHitCorrection != 0)
  {
    theHitCorrection->fillChargeSums(*col);
  }
  // Step B: Create empty output

  std::auto_ptr<HBHEDigiCollection> hbheResult(new HBHEDigiCollection());
  std::auto_ptr<HODigiCollection> hoResult(new HODigiCollection());
  std::auto_ptr<HFDigiCollection> hfResult(new HFDigiCollection());


  // Step C: Invoke the algorithm, passing in inputs and getting back outputs.
  theHBHEDigitizer->run(*col, *hbheResult);
  theHODigitizer->run(*col, *hoResult);
  theHFDigitizer->run(*col, *hfResult);

//  edm::LogInfo("HcalDigiProducer") << "HCAL HBHE digis : " << hbheResult->size();
//  edm::LogInfo("HcalDigiProducer") << "HCAL HO digis   : " << hoResult->size();
//  edm::LogInfo("HcalDigiProducer") << "HCAL HF digis   : " << hfResult->size();

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
      edm::LogError("HcalDigiProducer") << "Bad HcalHit subdetector " << subdet;
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

  theHBHEResponse->setGeometry(&*geometry);
  theHOResponse->setGeometry(&*geometry);
  theHFResponse->setGeometry(&*geometry);

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



#include <FWCore/Framework/interface/ESHandle.h>
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

#include "RecoEgamma/EgammaHLTProducers/interface/EcalListOfFEDSProducer.h"
#include "DataFormats/EcalRawData/interface/EcalListOfFEDS.h"

// Ecal Mapping 
#include "Geometry/EcalMapping/interface/EcalMappingRcd.h"
#include <DataFormats/FEDRawData/interface/FEDNumbering.h>

// Level 1 Trigger
#include "CondFormats/L1TObjects/interface/L1CaloGeometry.h"
#include "CondFormats/DataRecord/interface/L1CaloGeometryRecord.h"
                                                                                                                        
// EgammaCoreTools
#include "RecoEcal/EgammaCoreTools/interface/PositionCalc.h"
#include "DataFormats/Math/interface/RectangularEtaPhiRegion.h"

// Muon stuff
#include "DataFormats/L1GlobalMuonTrigger/interface/L1MuGMTExtendedCand.h"

#include <vector>

using namespace l1extra;

EcalListOfFEDSProducer::EcalListOfFEDSProducer(const edm::ParameterSet& pset) {
  
  debug_ = pset.getUntrackedParameter<bool>("debug");
  
  Pi0ListToIgnore_ = pset.getParameter<edm::InputTag>("Pi0ListToIgnore");
  
  EGamma_ = pset.getUntrackedParameter<bool>("EGamma",false);
  Muon_ = pset.getUntrackedParameter<bool>("Muon",false);
  Jets_ = pset.getUntrackedParameter<bool>("Jets",false);
  
  if (EGamma_ && Muon_) {
    throw cms::Exception("EcalListOfFEDSProducer") << 
      " Wrong configuration : EGamma and Muon should not be true at the same time." ;
  }
  
  
  if (EGamma_) {
    
    EMl1TagIsolated_    = consumes<L1EmParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("EM_l1TagIsolated"));
    EMl1TagNonIsolated_ = consumes<L1EmParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("EM_l1TagNonIsolated"));
    EMdoIsolated_       = pset.getUntrackedParameter<bool>("EM_doIsolated",  true);
    EMdoNonIsolated_    = pset.getUntrackedParameter<bool>("EM_doNonIsolated",true);
    EMregionEtaMargin_  = pset.getUntrackedParameter<double>("EM_regionEtaMargin", 0.25);
    EMregionPhiMargin_  = pset.getUntrackedParameter<double>("EM_regionPhiMargin", 0.40);
    Ptmin_iso_          = pset.getUntrackedParameter<double>("Ptmin_iso", 0.);
    Ptmin_noniso_       = pset.getUntrackedParameter<double>("Ptmin_noniso", 0.);
  }
  
  if (Muon_) {
    MUregionEtaMargin_ = pset.getUntrackedParameter<double>("MU_regionEtaMargin", 1.0);
    MUregionPhiMargin_ = pset.getUntrackedParameter<double>("MU_regionPhiMargin", 1.0);
    Ptmin_muon_        = pset.getUntrackedParameter<double>("Ptmin_muon", 0.);
    MuonSource_        = consumes<L1MuonParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("MuonSource"));
  }

 if (Jets_) {
   JETSregionEtaMargin_ = pset.getUntrackedParameter<double>("JETS_regionEtaMargin", 1.0);
   JETSregionPhiMargin_ = pset.getUntrackedParameter<double>("JETS_regionPhiMargin", 1.0);
   Ptmin_jets_          = pset.getUntrackedParameter<double>("Ptmin_jets", 0.);
   CentralSource_       = consumes<L1JetParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("CentralSource"));
   ForwardSource_       = consumes<L1JetParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("ForwardSource"));
   TauSource_           = consumes<L1JetParticleCollection>(pset.getUntrackedParameter<edm::InputTag>("TauSource"));
   JETSdoCentral_       = pset.getUntrackedParameter<bool>("JETS_doCentral", true);
   JETSdoForward_       = pset.getUntrackedParameter<bool>("JETS_doForward", true);
   JETSdoTau_           = pset.getUntrackedParameter<bool>("JETS_doTau", true);
 }

 OutputLabel_ = pset.getUntrackedParameter<std::string>("OutputLabel");
 
 TheMapping = new EcalElectronicsMapping();
 first_ = true;

 consumesMany<EcalListOfFEDS>();
 produces<EcalListOfFEDS>(OutputLabel_);
}

EcalListOfFEDSProducer::~EcalListOfFEDSProducer() {
  delete TheMapping;
}

void EcalListOfFEDSProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {

  edm::ParameterSetDescription desc;
  desc.add<bool>("debug", false);
 desc.add<edm::InputTag>("Pi0ListToIgnore", edm::InputTag(""));
 desc.add<bool>("EGamma",false);
 desc.add<bool>("Muon",false);
 desc.add<bool>("Jets",false);
 desc.add<edm::InputTag>("EM_l1TagIsolated", edm::InputTag(""));
 desc.add<edm::InputTag>("EM_l1TagNonIsolated", edm::InputTag(""));
 desc.add<bool>("EM_doIsolated",true);
 desc.add<bool>("EM_doNonIsolated",true);
 desc.add<double>("EM_regionEtaMargin",0.25);
 desc.add<double>("EM_regionPhiMargin",0.40);
 desc.add<double>("Ptmin_iso",0.);
 desc.add<double>("Ptmin_noniso",0.);
 desc.add<double>("MU_regionEtaMargin",1.0);
 desc.add<double>("MU_regionPhiMargin",1.0);
 desc.add<double>("Ptmin_muon",0.);
 desc.add<edm::InputTag>("MuonSource", edm::InputTag(""));
 desc.add<double>("JETS_regionEtaMargin",1.0);
 desc.add<double>("JETS_regionPhiMargin",1.0);
 desc.add<double>("Ptmin_jets",0.);
 desc.add<edm::InputTag>("CentralSource", edm::InputTag(""));
 desc.add<edm::InputTag>("ForwardSource", edm::InputTag(""));
desc.add<edm::InputTag>("TauSource", edm::InputTag(""));
 desc.add<bool>("JETS_doCentral",true);
 desc.add<bool>("JETS_doForward",true);
 desc.add<bool>("JETS_doTau",true);
desc.add<std::string>("OutputLabel", "");
  descriptions.add(("hltEcalListOfFEDSProducer"), desc);  
}

void EcalListOfFEDSProducer::beginJob()
{}

void EcalListOfFEDSProducer::endJob()
{}

void EcalListOfFEDSProducer::produce(edm::Event & e, const edm::EventSetup& iSetup){
  
  if (first_) {
    edm::ESHandle< EcalElectronicsMapping > ecalmapping;
    iSetup.get< EcalMappingRcd >().get(ecalmapping);
    const EcalElectronicsMapping* TheMapping_ = ecalmapping.product();
    *TheMapping = *TheMapping_;
    first_ = false;
  }                                                                                              
  
  auto productAddress = std::make_unique<EcalListOfFEDS>();
  
  std::vector<int> feds;		// the list of FEDS produced by this module
  
  
  // ---- First, get the list of ECAL FEDs which have already been unpacked
  
  std::vector< edm::Handle<EcalListOfFEDS> > FEDs_Done;
  std::vector<int> Done;
  e.getManyByType(FEDs_Done);
  unsigned int nDone = FEDs_Done.size();
  if (debug_) std::cout << " ECAL unpacking module has already run " << nDone << " times. " << std::endl;
  for (unsigned int id=0; id < nDone; id++) {
    // ignore the FEDs coming from unpacking in pi0 paths
    if( Pi0ListToIgnore_.label() == FEDs_Done[id].provenance()->moduleLabel() ){continue;}
    std::vector<int> done = FEDs_Done[id]-> GetList();
    for (int jd=0; jd < (int)done.size(); jd++) {
      Done.push_back(done[jd] - FEDNumbering::MINECALFEDID);
    }
  }
  if (debug_) std::cout << " For this event, " << Done.size() << " ECAL FEDs have already been unpacked." << std::endl;
  
  
  if (EGamma_) {
    // feds = Egamma(e, iSetup, Done);
    Egamma(e, iSetup, Done, feds);
  }
  
  if (Muon_) {
    // feds = Muon(e, iSetup, Done);
    Muon(e, iSetup, Done, feds);
  }
  
  if (Jets_) {
    // feds = Jets(e, iSetup, Done);
    Jets(e, iSetup, Done, feds);
  }
  
  if ( !EGamma_ && !Muon_ && ! Jets_)  {
    for (int i=1; i <= 54; i++) {
      if ( std::find(Done.begin(), Done.end(), i) == Done.end())
	feds.push_back(i);
    }
  }
  
  int nf = (int)feds.size();
  for (int i=0; i <nf; i++) {
    feds[i] += FEDNumbering::MINECALFEDID;
    if (debug_) std::cout << "Will unpack FED " << feds[i] << std::endl;
  }
  
  if (debug_ && nf < 1 ) 
    std::cout << " Warning : no ECAL FED to unpack for Run " << e.id().run() << "  Event " << e.id().event() << std::endl;
  
  productAddress.get() -> SetList(feds);
  e.put(std::move(productAddress),OutputLabel_);
}

void EcalListOfFEDSProducer::Egamma(edm::Event& e, const edm::EventSetup& es, std::vector<int>& done, std::vector<int>& FEDs ) {
  
  // std::vector<int> FEDs;
  
  if (debug_) std::cout << std::endl << std::endl << " enter in EcalListOfFEDSProducer::Egamma" << std::endl;
  
  //Get the L1 EM Particle Collection
  //Get the L1 EM Particle Collection
  edm::Handle<l1extra::L1EmParticleCollection > emIsolColl ;
  if(EMdoIsolated_)
    e.getByToken(EMl1TagIsolated_, emIsolColl);
  //Get the L1 EM Particle Collection
  edm::Handle<l1extra::L1EmParticleCollection > emNonIsolColl ;
  if (EMdoNonIsolated_)
    e.getByToken(EMl1TagNonIsolated_, emNonIsolColl);
  
  // Get the CaloGeometry
  edm::ESHandle<L1CaloGeometry> l1CaloGeom ;
  es.get<L1CaloGeometryRecord>().get(l1CaloGeom) ;
  
  if(EMdoIsolated_) {

    for( l1extra::L1EmParticleCollection::const_iterator emItr = emIsolColl->begin();
                        emItr != emIsolColl->end() ;++emItr ){

	float pt = emItr -> pt();
	if (pt < Ptmin_iso_ ) continue;
	if (debug_) std::cout << " Here is an L1 isoEM candidate of pt " << pt << std::endl;
        // Access the GCT hardware object corresponding to the L1Extra EM object.
        int etaIndex = emItr->gctEmCand()->etaIndex() ;
        int phiIndex = emItr->gctEmCand()->phiIndex() ;
        // Use the L1CaloGeometry to find the eta, phi bin boundaries.
        double etaLow  = l1CaloGeom->etaBinLowEdge( etaIndex ) ;
        double etaHigh = l1CaloGeom->etaBinHighEdge( etaIndex ) ;
        double phiLow  = l1CaloGeom->emJetPhiBinLowEdge( phiIndex ) ;
        double phiHigh = l1CaloGeom->emJetPhiBinHighEdge( phiIndex ) ;
	
	std::vector<int> feds = ListOfFEDS(etaLow, etaHigh, phiLow, phiHigh, EMregionEtaMargin_, EMregionPhiMargin_);
	for (int i=0; i < (int)feds.size(); i++) {
	  if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	       std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
	}
    } // end loop on L1EmParticleCollection
  }  // endif doIsolated_
  
  if (EMdoNonIsolated_) {
    
    for( l1extra::L1EmParticleCollection::const_iterator emItr = emNonIsolColl->begin();
	 emItr != emNonIsolColl->end() ;++emItr ){
      
      float pt = emItr -> pt();
      if (debug_) std::cout << " Here is an L1 nonisoEM candidate of pt " << pt << std::endl;
      if (pt < Ptmin_noniso_ ) continue;
      // Access the GCT hardware object corresponding to the L1Extra EM object.
      int etaIndex = emItr->gctEmCand()->etaIndex() ;
      int phiIndex = emItr->gctEmCand()->phiIndex() ;
      // std::cout << " etaIndex phiIndex " << etaIndex << " " << phiIndex << std::endl;
      // Use the L1CaloGeometry to find the eta, phi bin boundaries.
      double etaLow  = l1CaloGeom->etaBinLowEdge( etaIndex ) ;
      double etaHigh = l1CaloGeom->etaBinHighEdge( etaIndex ) ;
      double phiLow  = l1CaloGeom->emJetPhiBinLowEdge( phiIndex ) ;
      double phiHigh = l1CaloGeom->emJetPhiBinHighEdge( phiIndex ) ;
      
      std::vector<int> feds = ListOfFEDS(etaLow, etaHigh, phiLow, phiHigh, EMregionEtaMargin_, EMregionPhiMargin_);
      for (int i=0; i < (int)feds.size(); i++) {
	if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	     std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
	
      }
      
    } // end loop on L1EmParticleCollection
  }
  
  // std::cout << "end of get list of feds " << std::endl;
  
  if (debug_) {
    std::cout << std::endl;
    for (int i=0; i < (int)FEDs.size(); i++) {
      std::cout << "Egamma: unpack FED " << FEDs[i] << std::endl;
    }
    std::cout << "Number of FEDS is " << FEDs.size() << std::endl;
  }
  
  // return FEDs;
  
}

void EcalListOfFEDSProducer::Muon(edm::Event& e, const edm::EventSetup& es, std::vector<int>& done, std::vector<int>& FEDs) {
  
  // std::vector<int> FEDs;
  
  if (debug_) std::cout << std::endl << std::endl << " enter in EcalListOfFEDSProducer::Muon" << std::endl;
  
  edm::Handle<L1MuonParticleCollection> muColl;
  e.getByToken(MuonSource_, muColl);
    
  double epsilon = 0.01;
  
  for (L1MuonParticleCollection::const_iterator it=muColl->begin(); it != muColl->end(); it++) {
    const L1MuGMTExtendedCand muonCand = (*it).gmtMuonCand();
    double pt    =  (*it).pt();
    double eta   =  (*it).eta();
    double phi   =  (*it).phi();
    
    if (debug_) std::cout << " here is a L1 muon Seed  with (eta,phi) = " << 
		  eta << " " << phi << " and pt " << pt << std::endl;
    if (pt < Ptmin_muon_ ) continue;
    
    std::vector<int> feds = ListOfFEDS(eta, eta, phi-epsilon, phi+epsilon, MUregionEtaMargin_, MUregionPhiMargin_);
    
    for (int i=0; i < (int)feds.size(); i++) {
      if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	   std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
    }
  }
  
  if (debug_) {
    std::cout << std::endl;
    for (int i=0; i < (int)FEDs.size(); i++) {
      std::cout << "Muons: unpack FED " << FEDs[i] << std::endl;
    }
    std::cout << "Number of FEDS is " << FEDs.size() << std::endl;
  }
  
  // return FEDs;
  
}

void EcalListOfFEDSProducer::Jets(edm::Event& e, const edm::EventSetup& es, std::vector<int>& done, std::vector<int>& FEDs) {
  
  // std::vector<int> FEDs;
  
  if (debug_) std::cout << std::endl << std::endl << " enter in EcalListOfFEDSProducer::Jets" << std::endl;
  double epsilon = 0.01;
  
  if (JETSdoCentral_) {
    edm::Handle<L1JetParticleCollection> jetColl;
    e.getByToken(CentralSource_,jetColl);
    
    for (L1JetParticleCollection::const_iterator it=jetColl->begin(); it != jetColl->end(); it++) {
      
      double pt    =  it -> pt();
      double eta   =  it -> eta();
      double phi   =  it -> phi();
      
      if (debug_) std::cout << " here is a L1 CentralJet Seed  with (eta,phi) = " <<
		    eta << " " << phi << " and pt " << pt << std::endl;

      if (pt < Ptmin_jets_ ) continue;
      
      std::vector<int> feds = ListOfFEDS(eta, eta, phi-epsilon, phi+epsilon, JETSregionEtaMargin_, JETSregionPhiMargin_);
      
      for (int i=0; i < (int)feds.size(); i++) {
	if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	     std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
      }
    }
  }
  
  if (JETSdoForward_) {
    edm::Handle<L1JetParticleCollection> jetColl;
    e.getByToken(ForwardSource_,jetColl);
    
    for (L1JetParticleCollection::const_iterator it=jetColl->begin(); it != jetColl->end(); it++) {
      
      double pt    =  it -> pt();
      double eta   =  it -> eta();
      double phi   =  it -> phi();
      
      if (debug_) std::cout << " here is a L1 ForwardJet Seed  with (eta,phi) = " <<
		    eta << " " << phi << " and pt " << pt << std::endl;
      if (pt < Ptmin_jets_ ) continue;
      
      std::vector<int> feds = ListOfFEDS(eta, eta, phi-epsilon, phi+epsilon, JETSregionEtaMargin_, JETSregionPhiMargin_);
      
      for (int i=0; i < (int)feds.size(); i++) {
	if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	     std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
      }
    }
  }
  
  if (JETSdoTau_) {
    edm::Handle<L1JetParticleCollection> jetColl;
    e.getByToken(TauSource_,jetColl);
    
    for (L1JetParticleCollection::const_iterator it=jetColl->begin(); it != jetColl->end(); it++) {
      
      double pt    =  it -> pt();
      double eta   =  it -> eta();
      double phi   =  it -> phi();
      
      if (debug_) std::cout << " here is a L1 TauJet Seed  with (eta,phi) = " <<
		    eta << " " << phi << " and pt " << pt << std::endl;
      if (pt < Ptmin_jets_ ) continue;
      
      std::vector<int> feds = ListOfFEDS(eta, eta, phi-epsilon, phi+epsilon, JETSregionEtaMargin_, JETSregionPhiMargin_);
      
      for (int i=0; i < (int)feds.size(); i++) {
	if ( std::find(FEDs.begin(), FEDs.end(), feds[i]) == FEDs.end() &&
	     std::find(done.begin(), done.end(), feds[i]) == done.end() ) FEDs.push_back(feds[i]);
      }
    }
  }
  
  
  
  if (debug_) {
    std::cout << std::endl;
    for (int i=0; i < (int)FEDs.size(); i++) {
      std::cout << "Jets: unpack FED " << FEDs[i] << std::endl;
    }
    std::cout << "Number of FEDS is " << FEDs.size() << std::endl;
  }
  // return FEDs;
}


std::vector<int> EcalListOfFEDSProducer::ListOfFEDS(double etaLow, double etaHigh, double phiLow, 
					 double phiHigh, double etamargin, double phimargin) {
  
  std::vector<int> FEDs;
  
  if (phimargin > Geom::pi()) phimargin =  Geom::pi() ;
  
  
  if (debug_) std::cout << " etaLow etaHigh phiLow phiHigh " << etaLow << " " << 
		etaHigh << " " << phiLow << " " << phiHigh << std::endl;
  
  etaLow -= etamargin;
  etaHigh += etamargin;
  double phiMinus = phiLow - phimargin;
  double phiPlus = phiHigh + phimargin;
  
  bool all = false;
  double dd = fabs(phiPlus-phiMinus);
  if (debug_) std::cout << " dd = " << dd << std::endl;
  if (dd > 2.*Geom::pi() ) all = true;
  
  while (phiPlus > Geom::pi()) { phiPlus -= 2.*Geom::pi() ; }
  while (phiMinus < 0) { phiMinus += 2.*Geom::pi() ; }
  if ( phiMinus > Geom::pi()) phiMinus -= 2.*Geom::pi() ;
  
  double dphi = phiPlus - phiMinus;
  if (dphi < 0) dphi += 2.*Geom::pi() ;
  if (debug_) std::cout << "dphi = " << dphi << std::endl;
  if (dphi > Geom::pi()) {
    int fed_low1 = TheMapping -> GetFED(etaLow,phiMinus*180./Geom::pi());
    int fed_low2 = TheMapping -> GetFED(etaLow,phiPlus*180./Geom::pi());
    if (debug_) std::cout << "fed_low1 fed_low2 " << fed_low1 << " " << fed_low2 << std::endl;
    if (fed_low1 == fed_low2) all = true;
    int fed_hi1 = TheMapping -> GetFED(etaHigh,phiMinus*180./Geom::pi());
    int fed_hi2 = TheMapping -> GetFED(etaHigh,phiPlus*180./Geom::pi());
    if (debug_) std::cout << "fed_hi1 fed_hi2 " << fed_hi1 << " " << fed_hi2 << std::endl;
    if (fed_hi1 == fed_hi2) all = true;
  }
  
  
  if (all) {
    if (debug_) std::cout << " unpack everything in phi ! " << std::endl;
    phiMinus = -20 * Geom::pi() / 180.;  // -20 deg
    phiPlus = -40 * Geom::pi() / 180.;  // -20 deg
  }
  
  if (debug_) std::cout << " with margins : " << etaLow << " " << etaHigh << " " << 
		phiMinus << " " << phiPlus << std::endl;
  
  
  const RectangularEtaPhiRegion ecalregion(etaLow,etaHigh,phiMinus,phiPlus);
  
  FEDs = TheMapping -> GetListofFEDs(ecalregion);
  return FEDs;
}





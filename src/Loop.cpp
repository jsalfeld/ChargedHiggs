#include "interface/Loop.hpp"

#include <assert.h>
#include <iostream>

#include "NeroProducer/Core/interface/BareEvent.hpp"
#include "NeroProducer/Core/interface/BareJets.hpp"
#include "NeroProducer/Core/interface/BareMonteCarlo.hpp"
#include "NeroProducer/Core/interface/BareMet.hpp"
#include "NeroProducer/Core/interface/BareFatJets.hpp"
#include "NeroProducer/Core/interface/BareLeptons.hpp"
#include "NeroProducer/Core/interface/BareTaus.hpp"


//#define VERBOSE 1

int Looper::InitSmear(){
	int R=0;
	R+=AddSmear("NONE");
	R+=AddSmear("JES");
	R+=AddSmear("JER");
	return R;
}

int Looper::AddSmear(string name){
#ifdef VERBOSE
	if(VERBOSE>0)cout <<"[Looper]::[AddSmear] Adding Smear '"<<name<<"'."<<endl;
#endif
	if (name == "NONE" or name == ""){ SmearBase *s = new SmearBase(); systs_ . push_back(s); return 0;}
	if (name == "JES"){ SmearJes *s = new SmearJes(); systs_ . push_back(s); return 0;}
	if (name == "JER"){ SmearJer *s = new SmearJer(); systs_ . push_back(s); return 0;}

	cout <<"[Looper]::[AddSmear]::[WARNING] Smear "<<name<<" does NOT exist!!!"<<endl;
	return 1; // maybe throw exception
}

int Looper::InitTree()
{
	// --- declare branches, and structures
	BareEvent *e = new BareEvent(); 
		names_[ "Event" ] = bare_.size();
	bare_.push_back(e);
	// ---
	BareMonteCarlo *mc = new BareMonteCarlo(); 
		names_[ "MonteCarlo" ] = bare_.size();
	bare_.push_back(mc);
	// ---
	BareJets *j = new BareJets(); 
		names_[ "Jets" ] = bare_.size();
	bare_.push_back(j);
	// ---
	BareTaus *t = new BareTaus(); 
		names_[ "Taus" ] = bare_.size();
	bare_.push_back(t);
	// ---
	BareLeptons *l = new BareLeptons(); 
		names_[ "Leptons" ] = bare_.size();
	bare_.push_back(l);

	BareMet *met = new BareMet(); 
		names_[ "Met" ] = bare_.size();
	bare_.push_back(met);

	for (auto c : bare_ )
		c->setBranchAddresses(tree_);

	return 0;
}

void Looper::Loop()
{
	unsigned long nEntries = tree_->GetEntries();

	cout<<"[Looper]::[Loop]::[INFO] Running on "<<nEntries<<" entries" <<endl;

	for(unsigned long iEntry = 0 ;iEntry< nEntries ;++iEntry)
	{
	if(iEntry %10000 == 0 ) cout<<"[Looper]::[Loop]::[INFO] Getting Entry "<<iEntry<<" / "<<nEntries <<endl;

	#ifdef VERBOSE
		if (VERBOSE > 1) cout <<"[Looper]::[Loop] Getting Entry "<<iEntry << " of "<<nEntries<<endl;
	#endif
	// load tree
	tree_ -> GetEntry(iEntry);
	//move content into the event
	FillEvent();	
	// for each smearing
	for(auto s : systs_)
	  {
		#ifdef VERBOSE
			if (VERBOSE > 1) cout <<"[Looper]::[Loop] Doing syst "<< s -> name() <<endl;
		#endif
	  for(int i=-1; i<=1 ;++i)
		{
		if ( s->name().find("NONE") != string::npos and i!=0) continue;
		if ( s->name().find("NONE") == string::npos and i==0) continue; // for the 
		//reset 	
		event_->clearSyst();
		// 
		s->SetSyst(i);
		//smear
		s->smear(event_);
		//do the analysis
		for(auto a : analysis_)
			{
			#ifdef VERBOSE
				if (VERBOSE > 1) cout <<"[Looper]::[Loop] Doing Analysis "<<a->name()<<endl;;
			#endif
			// each analysis step will apply the SF accordingly to the object it is using
			event_ -> weight_ . clearSF() ;
			if ( a->analyze(event_,s->name()) > 0 ) break; // go on analyzing event, if no analysis returns >0
			}
		}
	  s->SetSyst(0); // not necessary, but cleaner in this way
	  }
	}
	// save output
	Write();
	Close();
	return;	
}

void Looper::ClearEvent(){

	event_ -> ClearEvent();

}

//#define VERBOSE 3

void Looper::FillEvent(){

	if ( tree_ -> GetTreeNumber() != fNumber)
		{
		fNumber = tree_->GetTreeNumber();
		// check name and weight TODO
		string fname = tree_->GetFile()->GetName();
		//"root://eoscms//store/../label/abc.root"
		cout<<"[Looper]::[FillEvent]::[INFO] Opening new file: "<<fname<<endl;
		size_t last = fname.rfind('/');
		size_t prevLast = fname.rfind('/',last-1);
		size_t eos = fname.find("/store/");
		string label=fname.substr(prevLast+1,last - 1 - prevLast ); //pos,len
		string dir =fname.substr(0,last); // remove the filename
		if (eos != string::npos) // strip out everything before /store/
			dir = dir.substr(eos, string::npos);
		// -- Load current MC --
		string savedDir=event_ -> weight_ . LoadMC( label );
		if (savedDir =="")
			{
			cout<<"[Looper]::[FillEvent]::[WARNING] failed to search MC by LABEL '"<<label<<"' search by dir '"<<dir<<"'"<<endl;
			// search for dir
			label = event_ -> weight_ . LoadMCbyDir(dir);
			savedDir = dir;
			cout<<"[Looper]::[FillEvent]::[WARNING] label found '"<<label<<"'"<<endl;
			}
		if ( dir != savedDir or label == "")
			cout<<"[Looper]::[FillEvent]::[ERROR] saved dir '"<<savedDir<<"' and current dir '"<< dir <<"' label '"<<label<<"'"<<endl;
		}

	BareEvent *e = dynamic_cast<BareEvent*> ( bare_ [names_["Event"] ] ) ; assert(e!=NULL);
#ifdef VERBOSE
	if(VERBOSE>0)cout <<"[Looper]::[FillEvent]::[INFO] Processing "<<e->runNum<<":"<<e->lumiNum<<":"<<e->eventNum<<endl;
#endif
	event_ -> isRealData_ = e->isRealData;

	e->clear();
	//fill Jets
#ifdef VERBOSE
	if(VERBOSE>1)cout <<"[Looper]::[FillEvent]::[DEBUG] Filling Jets: FIXME JES" <<endl;
#endif
	BareJets *bj = dynamic_cast<BareJets*> ( bare_ [ names_[ "Jets" ] ] ); assert (bj !=NULL);
	//cout <<"[Looper]::[FillEvent]::[DEBUG] "<<bj->p4->GetEntries()<<"|" << bj->bDiscr->size()<<endl;
	for (int iJet=0;iJet< bj -> p4 ->GetEntries() ; ++iJet)
		{
		Jet *j =new Jet();
		j->SetP4( *(TLorentzVector*) ((*bj->p4)[iJet]) );
		j->unc = 0.03; //bj -> unc -> at(iJet); FIXME 3% flat
		j->bdiscr = bj -> bDiscr -> at(iJet);
		event_ -> jets_ . push_back(j);
		}
	// Fill Leptons
	bj->clear();
#ifdef VERBOSE
	if(VERBOSE>1)cout <<"[Looper]::[FillEvent]::[DEBUG] Filling Leptons" <<endl;
#endif
	BareLeptons *bl = dynamic_cast<BareLeptons*> ( bare_[ names_["Leptons"] ]); assert(bl != NULL ) ;
	for (int iL = 0;iL<bl->p4->GetEntries() ;++iL)
		{
		Lepton *l = new Lepton();
		l->SetP4( *(TLorentzVector*) ((*bl->p4)[iL]) );
		l-> iso = (*bl->iso) [iL];
		l-> charge = ((*bl->pdgId)[iL] >0) ?  -1: 1; 
		l-> type = abs((*bl->pdgId)[iL]);
		event_ -> leps_ . push_back(l);
		}
	bl->clear();
	//Fill Tau
#ifdef VERBOSE
	if(VERBOSE>1)cout <<"[Looper]::[FillEvent]::[DEBUG] Filling Taus" <<endl;
#endif
	BareTaus *bt = dynamic_cast<BareTaus*> ( bare_[ names_["Taus"] ]); assert (bt != NULL ) ;
	for (int iL = 0;iL<bt->p4->GetEntries() ;++iL)
		{
		Tau *t = new Tau();
		t->SetP4( *(TLorentzVector*) ((*bt->p4)[iL]) );
		t-> iso = (*bt->iso) [iL];
		t-> charge = bt -> Q -> at(iL);
		t-> type = 15;
		event_ -> taus_ . push_back(t);
		}
	bt->clear();
	// FillMEt
#ifdef VERBOSE
	if(VERBOSE>1)cout <<"[Looper]::[FillEvent]::[DEBUG] Filling MET" <<endl;
#endif
	BareMet * met = dynamic_cast<BareMet*> ( bare_[ names_["Met"]]);

	if ( met->p4 ->GetEntries() != 1)
		cout<<"[Looper]::[FillEvent]::[ERROR] MET should have exactly 1 entry instead of "<<met->p4 ->GetEntries() <<endl;

	event_ -> met_ . SetP4 ( *(TLorentzVector*)(*met -> p4) [0]) ;
	event_ -> met_ . ptUp = met-> ptJESUP -> at(0);
	event_ -> met_ . ptDown = met-> ptJESDOWN -> at(0);
	met->clear();
#ifdef VERBOSE
	if(VERBOSE>0)cout <<"[Looper]::[FillEvent]::[DONE]"<<endl;
#endif 
}
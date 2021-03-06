#ifndef LOOP_H
#define LOOP_H

#include "TChain.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1D.h"


// Charged Higgs
#include "interface/AnalysisBase.hpp"
#include "interface/Smearer.hpp"
#include "interface/Event.hpp"

// Bare Structures
#include "NeroProducer/Core/interface/BareCollection.hpp"
#include "interface/Output.hpp"

#include "TStopwatch.h"

class Looper{

    private:
        TChain *tree_; 
        int fNumber;

        vector<BareCollection*> bare_;
        map<string,int> names_;

        vector<AnalysisBase*> analysis_;

        vector<SmearBase*> systs_;

        Event * event_;

        Output *output_;

        TStopwatch sw_;

    protected:
        // --- call by FillEvent
        void FillEventInfo();
        void FillJets();
        void FillLeptons();
        void FillTaus();
        void FillMet();
        void FillMC();
        //
        void NewFile();
        // ---- call by LOOP
        void FillEvent();
        void ClearEvent();
        void Write(){output_->Write();}	
        void Close(){output_->Close();}

    public:
        // -- constructor
        Looper();
        Looper(string chain);
        ~Looper(){ClearEvent();}
        // ---
        inline int AddToChain( string name ){ return tree_ -> Add( name.c_str() ) ; }
        inline int AddAnalysis( AnalysisBase* a ) {analysis_ . push_back(a); return 0;}
        inline int AddSmear(SmearBase *s) { systs_ .push_back(s) ; return 0; }
        int AddSmear(string name);
        int InitTree () ;
        int InitSmear() ;
        int InitAnalysis() { for(auto a : analysis_ ) { a->SetOutput(output_); a->Init() ;}  return 0;}
        int InitOutput(string name){output_ -> Open(name); return 0;}
        //
        void Loop();

        inline void AddMC( string label, string dir, double xsec, double nevents){event_ -> weight_. AddMC(label,dir,xsec,nevents); }
        inline void AddSF( string label, double sf, double err){ event_->weight_.AddSF(label,sf,err);}
        inline void AddPtEtaSF( string label, double pt1,double pt2 ,double eta1 ,double eta2,double sf, double err)
        {event_ -> weight_ .AddPtEtaSF(label,pt1,pt2,eta1,eta2,sf,err); }

        // -- PU Reweight
        inline void AddTarget( TH1*h, int runMin=-1, int runMax =-1,double lumi=-1){ event_ -> weight_ .AddTarget(h,runMin,runMax,lumi);}
        inline void AddTarget( TH1*h, string systName, int runMin=-1, int runMax =-1,double lumi=-1){ event_ ->weight_ . AddTarget(h,systName, runMin,runMax);}
        inline void AddPuMC( string label, TH1*h, int runMin=-1, int runMax =-1){ event_ ->weight_. AddMC(  label, h, runMin, runMax ); }


};

#endif
// Local Variables:
// mode:c++
// indent-tabs-mode:nil
// tab-width:4
// c-basic-offset:4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4 

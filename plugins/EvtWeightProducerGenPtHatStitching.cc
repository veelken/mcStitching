#include "HLTrigger/mcStitching/plugins/EvtWeightProducerGenPtHatStitching.h"

#include "FWCore/Utilities/interface/Exception.h"

#include <TArrayF.h>

#include <iostream>
#include <iomanip>
#include <algorithm> 

using namespace EvtWeightProducerGenPtHatStitching_namespace;

namespace
{
  bool
  isLowerPtHatBin(const sampleEntryType& sample1,
                  const sampleEntryType& sample2)
  {
    return sample1.pT_hat_bin_ < sample2.pT_hat_bin_;
  }

  template <typename T>
  std::string format_vT(const std::vector<T>& vT)
  {
    std::ostringstream os;

    os << "{ ";

    unsigned numEntries = vT.size();
    for ( unsigned iEntry = 0; iEntry < numEntries; ++iEntry ) {
      os << vT[iEntry];
      if ( iEntry < (numEntries - 1) ) os << ", ";
    }

    os << " }";
  
    return os.str();
  }

  std::string format_vdouble(const std::vector<double>& vd)
  {
    return format_vT(vd);
  }

  //std::string format_vint(const std::vector<int>& vi)
  //{
  //  return format_vT(vi);
  //}
}

EvtWeightProducerGenPtHatStitching::EvtWeightProducerGenPtHatStitching(const edm::ParameterSet& cfg) 
  : moduleLabel_(cfg.getParameter<std::string>("@module_label"))
  , histogram_X_(nullptr)
{
  std::cout << "<EvtWeightProducerGenPtHatStitching::EvtWeightProducerGenPtHatStitching (moduleLabel = " << moduleLabel_ << ")>:" << std::endl;

  src_genEventInfo_ = cfg.getParameter<edm::InputTag>("src_genEventInfo");
  token_genEventInfo_ = consumes<GenEventInfoProduct>(src_genEventInfo_);

  src_pileupSummaryInfo_ = cfg.getParameter<edm::InputTag>("src_pileupSummaryInfo");
  token_pileupSummaryInfo_ = consumes<std::vector<PileupSummaryInfo>>(src_pileupSummaryInfo_);

  binning_pT_hat_ = cfg.getParameter<std::vector<double>>("pT_hat_bins");
  numBins_pT_hat_ = binning_pT_hat_.size() - 1;
  if ( !(numBins_pT_hat_ >= 2) )
    throw cms::Exception("EvtWeightProducerGenPtHatStitching") 
      << " Invalid Configuration parameter 'pT_hat_bins' !!\n";

  edm::ParameterSet cfg_samples = cfg.getParameter<edm::ParameterSet>("samples");
  std::vector<std::string> sampleNames = cfg_samples.getParameterNamesForType<edm::ParameterSet>();
  for ( std::vector<std::string>::const_iterator sampleName = sampleNames.begin();
        sampleName != sampleNames.end(); ++sampleName ) {
    edm::ParameterSet cfg_sample = cfg_samples.getParameter<edm::ParameterSet>(*sampleName);
    sampleEntryType sample;
    sample.name_ = *sampleName;
    sample.crossSection_ = cfg_sample.getParameter<double>("crossSection");
    sample.numEvents_ = cfg_sample.getParameter<unsigned>("numEvents");
    sample.pT_hat_bin_ = cfg_sample.getParameter<int>("pT_hat_bin");
    samples_.push_back(sample);
  }
  std::sort(samples_.begin(), samples_.end(), isLowerPtHatBin);
  bool isInitialized_minbias = false;
  std::vector<bool> isInitialized_qcd(numBins_pT_hat_);
  for ( std::vector<sampleEntryType>::const_iterator sample = samples_.begin();
        sample != samples_.end(); ++sample ) {
    if ( sample->pT_hat_bin_ == -1 ) 
    {
      if ( isInitialized_minbias )
        throw cms::Exception("EvtWeightProducerGenPtHatStitching") 
          << " Invalid Configuration parameter 'samples' !!\n";
      sample_minbias_ = *sample;
      isInitialized_minbias = true;
    }
    else
    {
      if ( sample->pT_hat_bin_ >= 0 && sample->pT_hat_bin_ < numBins_pT_hat_ ) 
      {
        if ( isInitialized_qcd[sample->pT_hat_bin_] )
          throw cms::Exception("EvtWeightProducerGenPtHatStitching") 
            << " Invalid Configuration parameter 'samples' !!\n";
        samples_qcd_.push_back(*sample);
        isInitialized_qcd[sample->pT_hat_bin_] = true;
      }
      else
      {
        throw cms::Exception("EvtWeightProducerGenPtHatStitching") 
          << " Invalid Configuration parameter 'samples' !!\n";
      }
    }
  }
  for ( int idxBin = 0; idxBin < numBins_pT_hat_; ++idxBin )
  {
    if ( (idxBin == 0 &&  isInitialized_qcd[idxBin]) || 
         (idxBin >  0 && !isInitialized_qcd[idxBin]) )
      throw cms::Exception("EvtWeightProducerGenPtHatStitching") 
        << " Invalid Configuration parameter 'samples' !!\n";
  }

  double crossSection_minbias = sample_minbias_.crossSection_;
  std::vector<double> crossSections_qcd(numBins_pT_hat_);
  for ( std::vector<sampleEntryType>::const_iterator sample = samples_qcd_.begin();
        sample != samples_qcd_.end(); ++sample ) {
    crossSections_qcd[sample->pT_hat_bin_] = sample->crossSection_;
  }
  double crossSectionSum_qcd = 0.;
  for ( int idxBin = 1; idxBin < numBins_pT_hat_; ++idxBin )
  {
    crossSectionSum_qcd += crossSections_qcd[idxBin];
  }
  crossSections_qcd[0] = crossSection_minbias - crossSectionSum_qcd;
  assert(crossSections_qcd[0] > 0.);

  p_k_.resize(numBins_pT_hat_);
  for ( int idxBin = 0; idxBin < numBins_pT_hat_; ++idxBin )
  {
    p_k_[idxBin] = crossSections_qcd[idxBin]/crossSection_minbias;
    assert(p_k_[idxBin] >= 0. && p_k_[idxBin] <= 1.);
  }
  std::cout << " p_k = " << format_vdouble(p_k_) << std::endl;
    
  bxFrequency_ = cfg.getParameter<double>("bxFrequency");

  TArrayF binning(numBins_pT_hat_ + 1);
  for ( int idxBin = 0; idxBin < (numBins_pT_hat_ + 1); ++idxBin )
  {
    binning[idxBin] = binning_pT_hat_[idxBin];
  }
  histogram_X_ = new TH1I("X", "X", numBins_pT_hat_, binning.GetArray());

  produces<double>();
}

EvtWeightProducerGenPtHatStitching::~EvtWeightProducerGenPtHatStitching()
{
  delete histogram_X_;
}

void EvtWeightProducerGenPtHatStitching::produce(edm::Event& evt, const edm::EventSetup& es)
{
  //std::cout << "<EvtWeightProducerGenPtHatStitching::produce (moduleLabel = " << moduleLabel_ << ")>:" << std::endl;

  std::unique_ptr<double> evtWeight(new double());

  edm::Handle<GenEventInfoProduct> genEventInfoProduct;
  evt.getByToken(token_genEventInfo_, genEventInfoProduct);

  edm::Handle<std::vector<PileupSummaryInfo>> pileupSummaryInfos;
  evt.getByToken(token_pileupSummaryInfo_, pileupSummaryInfos);

  histogram_X_->Reset();

  if ( genEventInfoProduct.isValid() )
  {
    double genPtHat_hardscatter = genEventInfoProduct->qScale();
    histogram_X_->Fill(genPtHat_hardscatter);
  }
  
  for ( std::vector<PileupSummaryInfo>::const_iterator pileupSummaryInfo = pileupSummaryInfos->begin();
        pileupSummaryInfo != pileupSummaryInfos->end(); ++pileupSummaryInfo ) {
    if ( pileupSummaryInfo->getBunchCrossing() == 0 ) // CV: in-time pileup
    {
      const std::vector<float>& pileupSummaryInfo_genPtHat = pileupSummaryInfo->getPU_pT_hats();
      for ( std::vector<float>::const_iterator genPtHat_pileup = pileupSummaryInfo_genPtHat.begin();
            genPtHat_pileup != pileupSummaryInfo_genPtHat.end(); ++genPtHat_pileup ) {
        histogram_X_->Fill(*genPtHat_pileup);        
      }
    }
  }
  
  // CV: nomenclature (X, x_k, p_k, N) for multinominal distribution taken from
  //       https://en.wikipedia.org/wiki/Multinomial_distribution

  std::vector<int> x_k(numBins_pT_hat_);
  int N = 0;
  for ( int idxBin = 0; idxBin < numBins_pT_hat_; ++idxBin )
  {
    double histogram_X_binContent = histogram_X_->GetBinContent(idxBin + 1);
    assert(histogram_X_binContent >= 0);
    x_k[idxBin] = histogram_X_binContent;
    N += x_k[idxBin];
  }
  //std::cout << " x_k = " << format_vint(x_k) << " (N = " << N << ")" << std::endl;

  double rate_data = bxFrequency_;
  double expEvents_mc = 0.;
  for ( std::vector<sampleEntryType>::const_iterator sample = samples_.begin();
        sample != samples_.end(); ++sample ) {
    double prob_corr = 0.;
    //std::cout << "checking sample = " << sample->name_ << " (pT_hat_bin = " << sample->pT_hat_bin_ << ")" << std::endl;
    if ( sample->pT_hat_bin_ == -1 ) // minbias sample
    {
      prob_corr = 1.;
    }
    else // QCD sample
    {
      if ( (N*p_k_[sample->pT_hat_bin_]) > 0. )
      {
        prob_corr = x_k[sample->pT_hat_bin_]/(N*p_k_[sample->pT_hat_bin_]);
        //std::cout << " pT_hat_bin #" << sample->pT_hat_bin_ << ": prob_corr = " << prob_corr << std::endl;
      }
    }
    expEvents_mc += sample->numEvents_*prob_corr;
    //std::cout << "--> expEvents_mc = " << expEvents_mc << std::endl;
  }
  if ( expEvents_mc > 0. )
  {
    *evtWeight = rate_data/expEvents_mc;
  }
  else
  {
    *evtWeight = 0.;
  }
  //std::cout << "evtWeight = " << (*evtWeight) << std::endl;

  evt.put(std::move(evtWeight));
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(EvtWeightProducerGenPtHatStitching);

# mcStitching
Compute event weights that allow to combine minbias MC sample with QCD MC samples binned in pT^hat for Phase-2 HLT trigger studies

# To checkout the code:
Run

git clone https://github.com/veelken/mcStitching $CMSSW_BASE/src/HLTrigger/mcStitching

# To use the code:
You first need to define the minbias and QCD MC samples that you use.
Because a few events typically get lost during the Ntuple production (due to failed crab jobs),
you need to define the number of events contained in your Ntuples for the minbias sample and each QCD sample.
To do this, open the file $CMSSW_BASE/src/HLTrigger/mcStitching/python/stitchingWeight_cfi.py in the editor of your choice.
Then update the parameter 'numEvents' for each sample accordingly.
In case you use extention ("ext") samples, simply add the number of events contained in all samples that belong to the same pT^hat bin.

In case you want to add a further QCD sample, e.g. for the pT^hat bin 20-30 GeV, you need to:
- add the new QCD sample to the 'samples' PSet
- modify the parameter 'pT_hat_bins' accordingly
- enter the cross-section for the new QCD sample in the parameter 'crossSection'. Please take the cross-section from the twiki https://twiki.cern.ch/twiki/bin/viewauth/CMS/HighLevelTriggerPhase2 and divide the number that you find on the twiki by a factor 200 (the average pileup for the Phase-2 HLT trigger studies)
- update the parameter 'pT_hat_bin' for all QCD samples. Leave the parameter 'pT_hat_bin' for the minbias sample at its current value of -1. For the QCD samples, use the following numbering scheme: assign the QCD sample corresponding to the lowest pT^hat range the value +1 and increase the value by +1 as the pT^hat range increases.

In order to compute the stitching weights, add the following lines to your cfg.py file:

process.load("HLTrigger.mcStitching.stitchingWeight_cfi")

process.YOUR_CMS_SEQUENCE += process.stitchingWeight

The module stitchingWeight computes a double for each event. Apply this double as an event weight when you process the minbias and QCD samples.




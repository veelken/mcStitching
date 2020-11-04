import FWCore.ParameterSet.Config as cms

stitchingWeight = cms.EDProducer("EvtWeightProducerGenPtHatStitching",
    bxFrequency = cms.double(28000000.0),
    pT_hat_bins = cms.vdouble(
        0.0, 30.0, 50.0, 80.0, 120.0, 170.0, 300.0
    ),
    samples = cms.PSet(
        minbias = cms.PSet(
            crossSection = cms.double(400000000.0),
            numEvents = cms.uint32(981592),
            pT_hat_bin = cms.int32(-1)
        ),
        qcd_pt30to50 = cms.PSet(
            crossSection = cms.double(592000.0),
            numEvents = cms.uint32(483498),
            pT_hat_bin = cms.int32(1)
        ),
        qcd_pt50to80 = cms.PSet(
            crossSection = cms.double(88250.0),
            numEvents = cms.uint32(300000),
            pT_hat_bin = cms.int32(2)
        ),
        qcd_pt80to120 = cms.PSet(
            crossSection = cms.double(13355.0),
            numEvents = cms.uint32(100000),
            pT_hat_bin = cms.int32(3)
        ),
        qcd_pt120to170 = cms.PSet(
            crossSection = cms.double(2348.5),
            numEvents = cms.uint32(49601),
            pT_hat_bin = cms.int32(4)
        ),
        qcd_pt170to300 = cms.PSet(
            crossSection = cms.double(608.5),
            numEvents = cms.uint32(50000),
            pT_hat_bin = cms.int32(5)
        )
    ),
    src_genEventInfo = cms.InputTag("generator"),
    src_pileupSummaryInfo = cms.InputTag("addPileupInfo")
)

/**
 * \class L1uGtMuonCondition
 *
 *
 * Description: evaluation of a CondMuon condition.
 *
 * Implementation:
 *    <TODO: enter implementation details>
 *
 * \author: Vasile Mihai Ghete   - HEPHY Vienna
 *
 *
 */

// this class header
#include "L1Trigger/L1TGlobal/interface/L1uGtMuonCondition.h"

// system include files
#include <iostream>
#include <iomanip>

#include <string>
#include <vector>
#include <algorithm>

// user include files
//   base classes
#include "CondFormats/L1TObjects/interface/L1GtMuonTemplate.h"
#include "L1Trigger/GlobalTrigger/interface/L1GtConditionEvaluation.h"

#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutSetupFwd.h"

#include "DataFormats/L1Trigger/interface/Muon.h"

#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerFunctions.h"
#include "L1Trigger/L1TGlobal/interface/L1uGtBoard.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/MessageLogger/interface/MessageDrop.h"

// constructors
//     default
l1t::L1uGtMuonCondition::L1uGtMuonCondition() :
    L1GtConditionEvaluation() {

    // empty

}

//     from base template condition (from event setup usually)
l1t::L1uGtMuonCondition::L1uGtMuonCondition(const L1GtCondition* muonTemplate,
        const L1uGtBoard* ptrGTL, const int nrL1Mu,
        const int ifMuEtaNumberBits) :
    L1GtConditionEvaluation(),
    m_gtMuonTemplate(static_cast<const L1GtMuonTemplate*>(muonTemplate)),
    m_gtGTL(ptrGTL),
    m_ifMuEtaNumberBits(ifMuEtaNumberBits)
{
    m_corrParDeltaPhiNrBins = 0;
    m_condMaxNumberObjects = nrL1Mu;
}

// copy constructor
void l1t::L1uGtMuonCondition::copy(const l1t::L1uGtMuonCondition &cp) {

    m_gtMuonTemplate = cp.gtMuonTemplate();
    m_gtGTL = cp.gtGTL();

    m_ifMuEtaNumberBits = cp.gtIfMuEtaNumberBits();
    m_corrParDeltaPhiNrBins = cp.m_corrParDeltaPhiNrBins;

    m_condMaxNumberObjects = cp.condMaxNumberObjects();
    m_condLastResult = cp.condLastResult();
    m_combinationsInCond = cp.getCombinationsInCond();

    m_verbosity = cp.m_verbosity;

}

l1t::L1uGtMuonCondition::L1uGtMuonCondition(const l1t::L1uGtMuonCondition& cp) :
    L1GtConditionEvaluation() {
    copy(cp);
}

// destructor
l1t::L1uGtMuonCondition::~L1uGtMuonCondition() {

    // empty

}

// equal operator
l1t::L1uGtMuonCondition& l1t::L1uGtMuonCondition::operator= (const l1t::L1uGtMuonCondition& cp)
{
    copy(cp);
    return *this;
}

// methods
void l1t::L1uGtMuonCondition::setGtMuonTemplate(const L1GtMuonTemplate* muonTempl) {

    m_gtMuonTemplate = muonTempl;

}

///   set the pointer to GTL
void l1t::L1uGtMuonCondition::setGtGTL(const L1uGtBoard* ptrGTL) {

    m_gtGTL = ptrGTL;

}


//   set the number of bits for eta of muon objects
void l1t::L1uGtMuonCondition::setGtIfMuEtaNumberBits(
        const int& ifMuEtaNumberBitsValue) {

    m_ifMuEtaNumberBits = ifMuEtaNumberBitsValue;

}

//   set the maximum number of bins for the delta phi scales
void l1t::L1uGtMuonCondition::setGtCorrParDeltaPhiNrBins(
        const int& corrParDeltaPhiNrBins) {

    m_corrParDeltaPhiNrBins = corrParDeltaPhiNrBins;

}


// try all object permutations and check spatial correlations, if required
const bool l1t::L1uGtMuonCondition::evaluateCondition() const {  
 
    // BLW Need to pass this as an argument
    const int bxEval=0;   //BLW Change for BXVector

    // number of trigger objects in the condition
    int nObjInCond = m_gtMuonTemplate->nrObjects();

    // the candidates
    const BXVector<const l1t::Muon*>* candVec = m_gtGTL->getCandL1Mu();  //BLW Change for BXVector

    int numberObjects = candVec->size(bxEval);  //BLW Change for BXVector
    //LogTrace("L1GlobalTrigger") << "  numberObjects: " << numberObjects
    //    << std::endl;
    if (numberObjects < nObjInCond) {
        return false;
    }

    std::vector<int> index(numberObjects);

    for (int i = 0; i < numberObjects; ++i) {
        index[i] = i;
    }

    int jumpIndex = 1;
    int jump = factorial(numberObjects - nObjInCond);

    int totalLoops = 0;
    int passLoops = 0;

    // condition result condResult set to true if at least one permutation
    //     passes all requirements
    // all possible permutations are checked
    bool condResult = false;

    // store the indices of the muon objects
    // from the combination evaluated in the condition
    SingleCombInCond objectsInComb;
    objectsInComb.reserve(nObjInCond);

    // clear the m_combinationsInCond vector
    (combinationsInCond()).clear();

    do {

        if (--jumpIndex)
            continue;

        jumpIndex = jump;
        totalLoops++;

        // clear the indices in the combination
        objectsInComb.clear();

        bool tmpResult = true;

        // check if there is a permutation that matches object-parameter requirements
        for (int i = 0; i < nObjInCond; i++) {

            tmpResult &= checkObjectParameter(i,  *(candVec->at(bxEval,index[i]) )); //BLW Change for BXVector
            objectsInComb.push_back(index[i]);

        }

        // if permutation does not match particle conditions
        // skip charge correlation and spatial correlations
        if ( !tmpResult) {

            continue;

        }

        // get the correlation parameters (chargeCorrelation included here also)
        L1GtMuonTemplate::CorrelationParameter corrPar =
            *(m_gtMuonTemplate->correlationParameter());

        // charge_correlation consists of 3 relevant bits (D2, D1, D0)
        unsigned int chargeCorr = corrPar.chargeCorrelation;

        // charge ignore bit (D0) not set?
        if ((chargeCorr & 1) == 0) {

            for (int i = 0; i < nObjInCond; i++) {
                // check valid charge - skip if invalid charge
                int chargeValid = (candVec->at(bxEval,index[i]))->hwChargeValid(); //BLW Change for BXVector
                tmpResult &= chargeValid;

                if ( chargeValid==0) { //BLW type change for New Muon Class
                    continue;
                }
            }

            if ( !tmpResult) {
                continue;
            }

            if (nObjInCond == 1) { // one object condition

                // D2..enable pos, D1..enable neg
                if ( ! ( ( (chargeCorr & 4) != 0 && (candVec->at(bxEval,index[0]))->charge()> 0 )   //BLW Change for BXVector
                    || ( (chargeCorr & 2) != 0 &&   (candVec->at(bxEval,index[0]))->charge() < 0 ) )) {       //BLW Change for BXVector

                    continue;
                }

            }
            else { // more objects condition

                // find out if signs are equal
                bool equalSigns = true;
                for (int i = 0; i < nObjInCond-1; i++) {
                    if ((candVec->at(bxEval,index[i]))->charge() != (candVec->at(bxEval,index[i+1]))->charge()) { //BLW Change for BXVector
                        equalSigns = false;
                        break;
                    }
                }

                // two or three particle condition
                if (nObjInCond == 2 || nObjInCond == 3) {
                    // D2..enable equal, D1..enable not equal
                    if ( ! ( ( (chargeCorr & 4) != 0 && equalSigns ) || ( (chargeCorr & 2) != 0
                        && !equalSigns ) )) {

                        continue;
                    }
                }

                // four particle condition
                if (nObjInCond == 4) {
                    //counter to count positive charges to determine if there are pairs
                    unsigned int posCount = 0;

                    for (int i = 0; i < nObjInCond; i++) {
                        if ((candVec->at(bxEval,index[i]))->charge()> 0) {  //BLW Change for BXVector
                            posCount++;
                        }
                    }

                    // D2..enable equal, D1..enable pairs
                    if ( ! ( ( (chargeCorr & 4) != 0 && equalSigns ) || ( (chargeCorr & 2) != 0
                        && posCount == 2 ) )) {

                        continue;
                    }
                }
            }
        } // end signchecks


        if (m_gtMuonTemplate->wsc()) {

            // wsc requirements have always nObjInCond = 2
            // one can use directly index[0] and index[1] to compute
            // eta and phi differences
            const int ObjInWscComb = 2;
            if (nObjInCond != ObjInWscComb) {

                edm::LogError("L1GlobalTrigger") << "\n  Error: "
                    << "number of particles in condition with spatial correlation = " << nObjInCond
                    << "\n  it must be = " << ObjInWscComb << std::endl;
                // TODO Perhaps I should throw here an exception,
                // since something is really wrong if nObjInCond != ObjInWscComb (=2)
                continue;
            }

            unsigned int candDeltaEta;
            unsigned int candDeltaPhi;

            // check candDeltaEta

            // get eta index and the sign bit of the eta index (MSB is the sign)
            //   signedEta[i] is the signed eta index of (*candVec)[index[i]]
            int signedEta[ObjInWscComb];
            int signBit[ObjInWscComb] = { 0, 0 };

            int scaleEta = 1 << (m_ifMuEtaNumberBits - 1);

            for (int i = 0; i < ObjInWscComb; ++i) {
                signBit[i] = ((candVec->at(bxEval,index[i]))->hwEta() & scaleEta)>>(m_ifMuEtaNumberBits - 1);  //BLW Change for BXVector
                signedEta[i] = ((candVec->at(bxEval,index[i]))->hwEta() )%scaleEta;      //BLW Change for BXVector

                if (signBit[i] == 1) {
                    signedEta[i] = (-1)*signedEta[i];
                }

            }

            // compute candDeltaEta - add 1 if signs are different (due to +0/-0 indices)
            candDeltaEta = static_cast<int> (std::abs(signedEta[1] - signedEta[0]))
                + static_cast<int> (signBit[1]^signBit[0]);

            if ( !checkBit(corrPar.deltaEtaRange, candDeltaEta) ) {
                continue;
            }

            // check candDeltaPhi

            // calculate absolute value of candDeltaPhi
            if ((candVec->at(bxEval,index[0]))->hwPhi()> (candVec->at(bxEval,index[1]))->hwPhi()) {     //BLW Change for BXVector
                candDeltaPhi = (candVec->at(bxEval,index[0]))->hwPhi() - (candVec->at(bxEval,index[1]))->hwPhi();  //BLW Change for BXVector
            }
            else {
                candDeltaPhi = (candVec->at(bxEval,index[1]))->hwPhi() - (candVec->at(bxEval,index[0]))->hwPhi();   //BLW Change for BXVector
            }

            // check if candDeltaPhi > 180 (via delta_phi_maxbits)
            // delta_phi contains bits for 0..180 (0 and 180 included)
            // protect also against infinite loop...

            int nMaxLoop = 10;
            int iLoop = 0;

            while (candDeltaPhi >= m_corrParDeltaPhiNrBins) {

                unsigned int candDeltaPhiInitial = candDeltaPhi;

                // candDeltaPhi > 180 ==> take 360 - candDeltaPhi
                candDeltaPhi = (m_corrParDeltaPhiNrBins - 1) * 2 - candDeltaPhi;
                if (m_verbosity) {
                    LogTrace("L1GlobalTrigger")
                            << "    Initial candDeltaPhi = "
                            << candDeltaPhiInitial
                            << " > m_corrParDeltaPhiNrBins = "
                            << m_corrParDeltaPhiNrBins
                            << "  ==> candDeltaPhi rescaled to: "
                            << candDeltaPhi << " [ loop index " << iLoop
                            << "; breaks after " << nMaxLoop << " loops ]\n"
                            << std::endl;
                }

                iLoop++;
                if (iLoop > nMaxLoop) {
                    return false;
                }
            }

            // delta_phi bitmask is saved in two boost::uint64_t words
            if (candDeltaPhi < 64) {
                if (!checkBit(corrPar.deltaPhiRange0Word, candDeltaPhi) ) {
                    continue;
                }
            }
            else {
                if (!checkBit(corrPar.deltaPhiRange1Word, (candDeltaPhi - 64))) {
                    continue;
                }
            }

        } // end wsc check

        // if we get here all checks were successfull for this combination
        // set the general result for evaluateCondition to "true"

        condResult = true;
        passLoops++;
        (combinationsInCond()).push_back(objectsInComb);

    } while (std::next_permutation(index.begin(), index.end()) );

    //LogTrace("L1GlobalTrigger")
    //    << "\n  L1uGtMuonCondition: total number of permutations found:          " << totalLoops
    //    << "\n  L1uGtMuonCondition: number of permutations passing requirements: " << passLoops
    //    << "\n" << std::endl;

    return condResult;

}

// load muon candidates
const l1t::Muon* l1t::L1uGtMuonCondition::getCandidate(const int bx, const int indexCand) const {

    return (m_gtGTL->getCandL1Mu())->at(bx,indexCand);  //BLW Change for BXVector
}

/**
 * checkObjectParameter - Compare a single particle with a numbered condition.
 *
 * @param iCondition The number of the condition.
 * @param cand The candidate to compare.
 *
 * @return The result of the comparison (false if a condition does not exist).
 */

const bool l1t::L1uGtMuonCondition::checkObjectParameter(const int iCondition, const l1t::Muon& cand) const {

    // number of objects in condition
    int nObjInCond = m_gtMuonTemplate->nrObjects();

    if (iCondition >= nObjInCond || iCondition < 0) {
        return false;
    }

//     // empty candidates can not be compared
//     if (cand.empty()) {
//         return false;
//     }

    const L1GtMuonTemplate::ObjectParameter objPar =
        ( *(m_gtMuonTemplate->objectParameter()) )[iCondition];

    // using the logic table from GTL-9U-module.pdf
    // "Truth table for Isolation bit"

    // check thresholds:

    //   value < low pt threshold
    //       fail trigger

    //   low pt threshold <= value < high pt threshold & non-isolated muon:
    //       requestIso true:                    fail trigger
    //       requestIso false, enableIso true:   fail trigger
    //       requestIso false, enableIso false:  OK,  trigger

    //   low pt threshold <= value < high pt threshold & isolated muon:
    //       requestIso true:                    OK,  trigger
    //       requestIso false, enableIso true:   OK,  trigger
    //       requestIso false, enableIso false:  OK,  trigger

    //   value >= high pt threshold & non-isolated muon:
    //       requestIso true:  fail trigger
    //       requestIso false: OK,  trigger

    //   value >= high pt threshold & isolated muon:
    //       OK, trigger


    if ( !checkThreshold(objPar.ptHighThreshold, cand.hwPt(), m_gtMuonTemplate->condGEq()) ) {

        if ( !checkThreshold(objPar.ptLowThreshold, cand.hwPt(), m_gtMuonTemplate->condGEq()) ) {

            return false;
        }
        else {

            // check isolation
            if ( !cand.hwIso() ) {
                if (objPar.requestIso || objPar.enableIso) {

                    return false;
                }
            }

        }

    }
    else {

        if ( !cand.hwIso() ) {
            if (objPar.requestIso) {

                return false;
            }
        }

    }

    // check eta

    if (!checkBit(objPar.etaRange, cand.hwEta())) {
        return false;
    }

    // check phi  - in the requested range (no LUT used - LUT too big for hw chip)
    // for phiLow <= phiHigh takes [phiLow, phiHigh]
    // for phiLow >= phiHigh takes [phiLow, phiHigh] over zero angle!

    if (objPar.phiHigh >= objPar.phiLow) {

        if (! ( (objPar.phiLow <= (unsigned int)cand.hwPhi()) && ((unsigned int)cand.hwPhi() <= objPar.phiHigh ) )) {

            return false;
        }

    }
    else { // go over zero angle!!
        if (! ( (objPar.phiLow <= (unsigned int)cand.hwPhi()) || ((unsigned int)cand.hwPhi() <= objPar.phiHigh ) )) {

            return false;
        }
    }

    // check quality ( bit check )

    // A number of values is required to trigger (at least one).
    // "Don’t care" means that all values are allowed.
    // Qual = 000 means then NO MUON (GTL module)

    if (cand.hwQual() == 0) {
        return false;
    }

    if (objPar.qualityRange == 0) {
        return false;
    }
    else {
        if (!checkBit(objPar.qualityRange, cand.hwQual())) {
            return false;
        }
    }

    // check mip
    if (objPar.enableMip) {
        if (!cand.hwMip()) {

            return false;
        }
    }

    // particle matches if we get here
    //LogTrace("L1GlobalTrigger")
    //    << "  checkObjectParameter: muon object OK, passes all requirements\n" << std::endl;

    return true;
}

void l1t::L1uGtMuonCondition::print(std::ostream& myCout) const {

    m_gtMuonTemplate->print(myCout);

    myCout << "    Number of bits for eta of muon objects = "
            << m_ifMuEtaNumberBits << std::endl;
    myCout << "    Maximum number of bins for the delta phi scales = "
            << m_corrParDeltaPhiNrBins << "\n " << std::endl;

    L1GtConditionEvaluation::print(myCout);

}


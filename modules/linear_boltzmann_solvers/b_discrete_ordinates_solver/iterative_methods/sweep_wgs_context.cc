#include "modules/linear_boltzmann_solvers/b_discrete_ordinates_solver/iterative_methods/sweep_wgs_context.h"

#include <petscksp.h>

#include "modules/linear_boltzmann_solvers/b_discrete_ordinates_solver/lbs_discrete_ordinates_solver.h"
#include "modules/linear_boltzmann_solvers/a_lbs_solver/preconditioning/lbs_shell_operations.h"

#include "framework/runtime.h"
#include "framework/logging/log.h"

#include <iomanip>

#define sc_double static_cast<double>
#define PCShellPtr PetscErrorCode (*)(PC, Vec, Vec)

namespace lbs
{

SweepWGSContext::SweepWGSContext(
  DiscreteOrdinatesSolver& lbs_solver,
  LBSGroupset& groupset,
  const SetSourceFunction& set_source_function,
  SourceFlags lhs_scope,
  SourceFlags rhs_scope,
  bool log_info,
  std::shared_ptr<chi_mesh::sweep_management::SweepChunk> sweep_chunk)
  : WGSContext(lbs_solver, groupset, set_source_function, lhs_scope, rhs_scope, log_info),
    sweep_chunk_(std::move(sweep_chunk)),
    sweep_scheduler_(lbs_solver.SweepType() == "AAH"
                       ? chi_mesh::sweep_management::SchedulingAlgorithm::DEPTH_OF_GRAPH
                       : chi_mesh::sweep_management::SchedulingAlgorithm::FIRST_IN_FIRST_OUT,
                     *groupset.angle_agg_,
                     *sweep_chunk_),
    lbs_ss_solver_(lbs_solver)
{
}

void
SweepWGSContext::PreSetupCallback()
{
  if (log_info_)
  {
    std::string method_name;
    switch (groupset_.iterative_method_)
    {
      case IterativeMethod::KRYLOV_RICHARDSON:
        method_name = "KRYLOV_RICHARDSON";
        break;
      case IterativeMethod::KRYLOV_GMRES:
        method_name = "KRYLOV_GMRES";
        break;
      case IterativeMethod::KRYLOV_BICGSTAB:
        method_name = "KRYLOV_BICGSTAB";
        break;
      default:
        method_name = "KRYLOV_GMRES";
    }
    Chi::log.Log() << "\n\n"
                   << "********** Solving groupset " << groupset_.id_ << " with " << method_name
                   << ".\n\n"
                   << "Quadrature number of angles: " << groupset_.quadrature_->abscissae_.size()
                   << "\n"
                   << "Groups " << groupset_.groups_.front().id_ << " "
                   << groupset_.groups_.back().id_ << "\n\n";
  }
}

void
SweepWGSContext::SetPreconditioner(KSP& solver)
{
  auto& ksp = solver;

  PC pc;
  KSPGetPC(ksp, &pc);

  if (groupset_.apply_wgdsa_ or groupset_.apply_tgdsa_)
  {
    PCSetType(pc, PCSHELL);
    PCShellSetApply(pc, (PCShellPtr)WGDSA_TGDSA_PreConditionerMult);
    PCShellSetContext(pc, &(*this));
  }

  KSPSetPCSide(ksp, PC_LEFT);
  KSPSetUp(ksp);
}

std::pair<int64_t, int64_t>
SweepWGSContext::SystemSize()
{
  const size_t local_node_count = lbs_solver_.LocalNodeCount();
  const size_t globl_node_count = lbs_solver_.GlobalNodeCount();
  const size_t num_moments = lbs_solver_.NumMoments();

  const size_t groupset_numgrps = groupset_.groups_.size();
  const auto num_delayed_psi_info = groupset_.angle_agg_->GetNumDelayedAngularDOFs();
  const size_t local_size =
    local_node_count * num_moments * groupset_numgrps + num_delayed_psi_info.first;
  const size_t globl_size =
    globl_node_count * num_moments * groupset_numgrps + num_delayed_psi_info.second;
  const size_t num_angles = groupset_.quadrature_->abscissae_.size();
  const size_t num_psi_global = globl_node_count * num_angles * groupset_.groups_.size();
  const size_t num_delayed_psi_globl = num_delayed_psi_info.second;

  if (log_info_)
  {
    Chi::log.Log() << "Total number of angular unknowns: " << num_psi_global << "\n"
                   << "Number of lagged angular unknowns: " << num_delayed_psi_globl << "("
                   << std::setprecision(2)
                   << sc_double(num_delayed_psi_globl) * 100 / sc_double(num_psi_global) << "%)";
  }

  return {static_cast<int64_t>(local_size), static_cast<int64_t>(globl_size)};
}

void
SweepWGSContext::ApplyInverseTransportOperator(SourceFlags scope)
{
  ++counter_applications_of_inv_op_;
  const bool use_bndry_source_flag =
    (scope & APPLY_FIXED_SOURCES) and (not lbs_solver_.Options().use_src_moments);

  sweep_scheduler_.SetBoundarySourceActiveFlag(use_bndry_source_flag);

  if (scope & ZERO_INCOMING_DELAYED_PSI) sweep_scheduler_.ZeroIncomingDelayedPsi();

  // Sweep
  sweep_scheduler_.ZeroOutputFluxDataStructures();
  sweep_scheduler_.Sweep();
}

void
SweepWGSContext::PostSolveCallback()
{
  // Perform final sweep with converged phi and delayed psi dofs
  if (groupset_.iterative_method_ != IterativeMethod::KRYLOV_RICHARDSON)
  {
    lbs_ss_solver_.ZeroOutflowBalanceVars(groupset_);

    const auto scope = lhs_src_scope_ | rhs_src_scope_;

    set_source_function_(groupset_, lbs_solver_.QMomentsLocal(), lbs_solver_.PhiOldLocal(), scope);
    sweep_scheduler_.SetDestinationPhi(lbs_solver_.PhiNewLocal());

    ApplyInverseTransportOperator(scope);

    lbs_solver_.GSScopedCopyPrimarySTLvectors(
      groupset_, PhiSTLOption::PHI_NEW, PhiSTLOption::PHI_OLD);
  }

  // Print solution info
  {
    double sweep_time = sweep_scheduler_.GetAverageSweepTime();
    double chunk_overhead_ratio = 1.0 - sweep_scheduler_.GetAngleSetTimings()[2];
    double source_time = Chi::log.ProcessEvent(lbs_solver_.GetSourceEventTag(),
                                               chi::ChiLog::EventOperation::AVERAGE_DURATION);
    size_t num_angles = groupset_.quadrature_->abscissae_.size();
    size_t num_unknowns = lbs_solver_.GlobalNodeCount() * num_angles * groupset_.groups_.size();

    if (log_info_)
    {
      Chi::log.Log() << "\n\n";
      Chi::log.Log() << "        Set Src Time/sweep (s):        " << source_time;
      Chi::log.Log() << "        Average sweep time (s):        " << sweep_time;
      Chi::log.Log() << "        Chunk-Overhead-Ratio  :        " << chunk_overhead_ratio;
      Chi::log.Log() << "        Sweep Time/Unknown (ns):       "
                     << sweep_time * 1.0e9 * Chi::mpi.process_count /
                          static_cast<double>(num_unknowns);
      Chi::log.Log() << "        Number of unknowns per sweep:  " << num_unknowns;
      Chi::log.Log() << "\n\n";

      std::string sweep_log_file_name = std::string("GS_") + std::to_string(groupset_.id_) +
                                        std::string("_SweepLog_") +
                                        std::to_string(Chi::mpi.location_id) + std::string(".log");
      groupset_.PrintSweepInfoFile(sweep_scheduler_.SweepEventTag(), sweep_log_file_name);
    }
  }
}

} // namespace lbs

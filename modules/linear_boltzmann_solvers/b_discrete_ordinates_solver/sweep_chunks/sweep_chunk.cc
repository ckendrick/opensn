#include "modules/linear_boltzmann_solvers/b_discrete_ordinates_solver/sweep_chunks/sweep_chunk.h"

#include "modules/linear_boltzmann_solvers/a_lbs_solver/groupset/lbs_groupset.h"
#include "framework/math/spatial_discretization/finite_element/piecewise_linear/piecewise_linear_discontinuous.h"

#include "framework/runtime.h"
#include "framework/logging/log.h"
#include "framework/logging/log_exceptions.h"

namespace opensn
{
namespace lbs
{

SweepChunk::SweepChunk(std::vector<double>& destination_phi,
                       std::vector<double>& destination_psi,
                       const MeshContinuum& grid,
                       const SpatialDiscretization& discretization,
                       const std::vector<UnitCellMatrices>& unit_cell_matrices,
                       std::vector<lbs::CellLBSView>& cell_transport_views,
                       const std::vector<double>& source_moments,
                       const LBSGroupset& groupset,
                       const std::map<int, std::shared_ptr<MultiGroupXS>>& xs,
                       int num_moments,
                       int max_num_cell_dofs,
                       std::unique_ptr<SweepDependencyInterface> sweep_dependency_interface_ptr)
  : opensn::SweepChunk(destination_phi, destination_psi),
    grid_(grid),
    grid_fe_view_(discretization),
    unit_cell_matrices_(unit_cell_matrices),
    grid_transport_view_(cell_transport_views),
    q_moments_(source_moments),
    groupset_(groupset),
    xs_(xs),
    num_moments_(num_moments),
    save_angular_flux_(not destination_psi.empty()),
    sweep_dependency_interface_ptr_(std::move(sweep_dependency_interface_ptr)),
    sweep_dependency_interface_(*sweep_dependency_interface_ptr_),
    groupset_angle_group_stride_(groupset_.psi_uk_man_.NumberOfUnknowns() *
                                 groupset_.groups_.size()),
    groupset_group_stride_(groupset_.groups_.size())
{
  Amat_.resize(max_num_cell_dofs, std::vector<double>(max_num_cell_dofs));
  Atemp_.resize(max_num_cell_dofs, std::vector<double>(max_num_cell_dofs));
  b_.resize(groupset.groups_.size(), std::vector<double>(max_num_cell_dofs, 0.0));
  source_.resize(max_num_cell_dofs, 0.0);

  sweep_dependency_interface_.groupset_angle_group_stride_ = groupset_angle_group_stride_;
  sweep_dependency_interface_.groupset_group_stride_ = groupset_group_stride_;
}

void
SweepChunk::OutgoingSurfaceOperations()
{
  const size_t f = sweep_dependency_interface_.current_face_idx_;
  const auto& IntF_shapeI = (*IntS_shapeI_)[f];
  const double mu = face_mu_values_[f];
  const double wt = direction_qweight_;

  const bool on_boundary = sweep_dependency_interface_.on_boundary_;
  const bool is_reflecting_boundary = sweep_dependency_interface_.is_reflecting_bndry_;

  const size_t num_face_nodes = cell_mapping_->NumFaceNodes(f);
  for (int fi = 0; fi < num_face_nodes; ++fi)
  {
    const int i = cell_mapping_->MapFaceNode(f, fi);

    double* psi = sweep_dependency_interface_.GetDownwindPsi(fi);

    if (psi != nullptr)
      if (not on_boundary or is_reflecting_boundary)
        for (int gsg = 0; gsg < gs_ss_size_; ++gsg)
          psi[gsg] = b_[gsg][i];
    if (on_boundary and not is_reflecting_boundary)
      for (int gsg = 0; gsg < gs_ss_size_; ++gsg)
        cell_transport_view_->AddOutflow(gs_gi_ + gsg, wt * mu * b_[gsg][i] * IntF_shapeI[i]);

  } // for fi
}

void
SweepDependencyInterface::SetupIncomingFace(
  int face_id, size_t num_face_nodes, uint64_t neighbor_id, bool on_local_face, bool on_boundary)
{
  current_face_idx_ = face_id;
  num_face_nodes_ = num_face_nodes;
  neighbor_id_ = neighbor_id;
  on_local_face_ = on_local_face;
  on_boundary_ = on_boundary;
}

void
SweepDependencyInterface::SetupOutgoingFace(int face_id,
                                            size_t num_face_nodes,
                                            uint64_t neighbor_id,
                                            bool on_local_face,
                                            bool on_boundary,
                                            int locality)
{
  current_face_idx_ = face_id;
  num_face_nodes_ = num_face_nodes;
  neighbor_id_ = neighbor_id;
  face_locality_ = locality;
  on_local_face_ = on_local_face;
  on_boundary_ = on_boundary;

  is_reflecting_bndry_ =
    (on_boundary_ and angle_set_->GetBoundaries()[neighbor_id_]->IsReflecting());
}

} // namespace lbs
} // namespace opensn

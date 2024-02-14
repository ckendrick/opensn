--############################################### Setup mesh
nodes={}
N=100
L=2.0
xmin = -L/2
dx = L/N
for i=1,(N+1) do
    k=i-1
    nodes[i] = xmin + k*dx
end

meshgen1 = mesh.OrthogonalMeshGenerator.Create
({
  node_sets = {nodes,nodes}
})
mesh.MeshGenerator.Execute(meshgen1)

--############################################### Set Material IDs
mesh.SetUniformMaterialID(0)

unit_sim_tests.SimTest02_FV();
MPIBarrier()
if (location_id == 0) then
    os.execute("rm CodeTut2_FV*")
end

import pytest
import os
import sys
import numpy as np
from pymatgen.io.vasp.inputs import Poscar
from pymatgen.io.vasp.outputs import Vasprun
from flare.struc import Structure, get_unique_species
from flare.dft_interface.vasp_util import parse_dft_forces, run_dft, \
    edit_dft_input_positions, dft_input_to_structure,\
    parse_dft_forces_and_energy, md_trajectory_from_vasprun

def cleanup_vasp_run(target: str = None):
    os.system('rm POSCAR')
    os.system('rm vasprun.xml')

# ------------------------------------------------------
#                   test  otf helper functions
# ------------------------------------------------------

@pytest.mark.parametrize("poscar",
                         [
                             ('./test_files/test_POSCAR')
                         ]
                         )
def test_structure_parsing(poscar):
    structure = dft_input_to_structure(poscar)
    pmg_struct = Poscar.from_file(poscar).structure
    assert len(structure.species_labels) == len(pmg_struct)
    assert (structure.cell == pmg_struct.lattice.matrix).all()
    for i, spec in enumerate(structure.species_labels):
        assert spec == pmg_struct[i].specie.symbol
    assert np.isclose(structure.positions, pmg_struct.cart_coords).all()

@pytest.mark.parametrize("poscar",
                         [
                            ('./test_files/test_POSCAR')
                         ]
                         )
def test_input_to_structure(poscar):
    assert isinstance(dft_input_to_structure(poscar), Structure)    

@pytest.mark.parametrize('cmd, poscar',
                         [
                            ('python ./test_files/dummy_vasp.py',
                            './test_files/test_POSCAR')
                         ]
                         )
def test_vasp_calling(cmd, poscar):

    cleanup_vasp_run()

    structure = dft_input_to_structure(poscar)

    forces1 = run_dft('.', cmd, structure=structure, en=False)
    forces2, energy2 = run_dft('.', cmd, structure=structure, en=True)
    forces3 = parse_dft_forces('./test_files/test_vasprun.xml')
    forces4, energy4 = parse_dft_forces_and_energy('./test_files/test_vasprun.xml')

    vr_step = Vasprun('./test_files/test_vasprun.xml').ionic_steps[-1]
    ref_forces = vr_step['forces']
    ref_energy = vr_step['electronic_steps'][-1]['e_0_energy']

    assert len(forces1) == len(ref_forces)
    assert len(forces2) == len(ref_forces)
    assert len(forces3) == len(ref_forces)
    assert len(forces4) == len(ref_forces)

    for i in range(structure.nat):
        assert np.isclose(forces1[i], ref_forces[i]).all()
        assert np.isclose(forces2[i], ref_forces[i]).all()
        assert np.isclose(forces3[i], ref_forces[i]).all()
        assert np.isclose(forces4[i], ref_forces[i]).all()
        assert energy2 == ref_energy
        assert energy4 == ref_energy

    cleanup_vasp_run()


@pytest.mark.parametrize('cmd, poscar',
                         [
                            ('python ./test_files/dummy_vasp.py test_fail',
                            './test_files/test_POSCAR')
                         ]
                         )
def test_vasp_calling_fail(cmd, poscar):

    structure = dft_input_to_structure(poscar)

    caught_error = False
    try:
        forces = run_dft('.', cmd, structure=structure, en=False)
    except FileNotFoundError:
        caught_error = True
    assert caught_error


def test_vasp_input_edit():
    os.system('cp test_files/test_POSCAR ./POSCAR')
    structure = dft_input_to_structure('./test_files/test_POSCAR')

    structure.vec1 += np.random.randn(3)
    structure.positions[0] += np.random.randn(3)

    new_file = edit_dft_input_positions('./POSCAR', structure=structure)

    final_structure = dft_input_to_structure(new_file)

    assert np.isclose(final_structure.vec1, structure.vec1).all()
    assert np.isclose(final_structure.positions[0], structure.positions[0]).all()

    os.system('rm ./POSCAR')
    os.system('rm ./POSCAR.bak')

def test_md_trajectory():
    structures = md_trajectory_from_vasprun('test_files/test_vasprun.xml')
    assert len(structures) == 2
    for struct in structures:
        assert struct.forces.shape == (6,3)
        assert struct.energy is not None
        assert struct.stress.shape == (3,3)
    structures = md_trajectory_from_vasprun('test_files/test_vasprun.xml', ionic_step_skips=2)
    assert len(structures) == 1


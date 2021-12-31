"""
This module is to provide the same interface as the module `dft_interface`, so we can use ASE atoms and calculators to run OTF
"""
import os
import numpy as np
from copy import deepcopy


def parse_dft_input(atoms):
    pos = np.copy(atoms.positions)
    spc = atoms.get_chemical_symbols()
    cell = np.array(atoms.get_cell())

    # build mass dict
    mass = atoms.get_masses()
    mass_dict = {}
    for i in range(len(spc)):
        spec_ind = str(spc[i])
        if spec_ind not in mass_dict.keys():
            mass_dict[spec_ind] = mass[i]
    return pos, spc, cell, mass_dict


def run_dft_par(atoms, structure, dft_calc, dft_kwargs, **kwargs):
    """
    Assume that the atoms have been updated
    """
    # change from FLARE to DFT calculator
    calc = deepcopy(dft_calc)
    atoms.set_calculator(calc)

    # clean up previous results
    if dft_kwargs.get("cleanup", None):
        if "dft" in os.listdir():
            if "WAVECAR" in os.listdir("dft"):
                os.remove("dft/WAVECAR")
        if "WAVECAR" in os.listdir():
            os.remove("WAVECAR")

    # Calculate DFT energy, forces, and stress.
    # Source code for DFT parser:
    # https://wiki.fysik.dtu.dk/ase/_modules/ase/io/espresso.html#read_espresso_out
    # Note that ASE and QE stresses differ by a minus sign.
    forces = atoms.get_forces()
    stress = atoms.get_stress()
    energy = atoms.get_potential_energy()
    # The results will be written to the 'results' dictionary, and structure.forces
    # directly return the atoms.calc.results['forces']

    structure.calc.results = atoms.calc.results

    return forces

from pathlib import Path

from ase.io import read
import ace

at = read(Path(__file__).parent / "../assets/ch_test2.xyz")

ace_version = 1
cace_calc = ace.ACECalculator(jsonpath=Path(__file__).parent / f"../assets/CH_ace_test_ACE{ace_version}.json", ACE_version=ace_version)


at.set_calculator(cace_calc)

print("Positions of the atoms")
print(at.get_positions())

print("TEST energy call")
print(at.get_potential_energy())

print("TEST force call")
print(at.get_forces())

at.pbc = [True, True, True]
print("TEST stress call")
print(at.get_stress())

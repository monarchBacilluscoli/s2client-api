# Thoughts and Ideas

scared!**The implementation of my rolling DE has a big problem!**
The implmentation before:
1. **Evaluate** the **main pop** and sort
2. **Offsprings** are bred from the parent pop(main pop)
3. **Evaluate and Sort** the **offspring** pop only
4. **concrete** the offspring pop into parent pop
5. **Shrink** the mixed pop.

The implmentation after(Elite):
1. **Evaluate** the **main pop** and sort
1. Reserve only a part of the parent pop as parents(**Elite**)
1. Breed the rest part by Elites
1. Evaluate them all and sort()
1. **Sort** and shirk again.

## Difference
1. Evlauate pop\_size, reserve elite\_size(20% pop\_size) (evaluate all, keep a part)

all the other things are the same.

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

after finish the re-implementation, I need to do something else:
1. change the MAX\_SIM\_NUMBER to a larger value (80, for example)
2. check if the run is the same as what I thought.
3. compare the differences between the two runs.

## The weired problem
If the population has been shuffled, the evaluation value of the whole pop will get worse and worse.
! Even though the breed doesn't function.
The most depressing thing is that there is still a possible reason leading to this condition, that is if you shuffled the population, the mapping relationship will change, then the evaluation value will unstably change, too.

**Stupid program. Stupid research topic.**

I know! I will try to use a enemy pop which only contains copies of one solution. Then I will see whether or not the population of mine evolves worse.

// this class is used for controling game process so as I can optimize offline

#ifndef PROBLEM_H
#define PROBLEM_H

#include<vector>

//! Here I can add a default template parameter value
template <typename T>
class problem {
public:
    problem();
    ~problem();
    std::vector<float> evaluate(T solution);
private:

};

#endif // !PROBLEM_H


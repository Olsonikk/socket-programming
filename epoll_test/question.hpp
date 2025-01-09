#pragma once

#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

class Question
{
public:
    // Konstruktor
    Question();

    // Gettery
    std::string getQuestionText() const;
    std::vector<std::string> getOptions() const;

    // Metody do generowania pytań
    void generateQuestion();
    void generateAnswer();
    int getAnswer() const;
    // Operator do wyświetlania pytania i zwracania poprawnej odpowiedzi
    int operator()();

private:
    std::string text;
    std::vector<std::string> options;
    int correctAnswer;

    // Atrybuty do przechowywania operacji
    int operand1;
    int operand2;
    char operation; // '+', '-', '*'

    // Metoda pomocnicza do generowania losowej operacji
    char getRandomOperation() const;
};
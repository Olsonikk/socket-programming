#pragma once

#include <string>

class Question
{
public:
    // Konstruktor
    Question();

    // Gettery
    std::string getQuestionText() const;

    // Metody do generowania pytań
    void generateQuestion();
    void generateAnswer();
    int getAnswer() const;
    // Operator do wyświetlania pytania i zwracania poprawnej odpowiedzi
    int operator()();

private:
    std::string text;
    int correctAnswer;

    // Atrybuty do przechowywania operacji
    int operand1;
    int operand2;
    char operation; // '+', '-', '*'

    // Metoda pomocnicza do generowania losowej operacji
    char getRandomOperation() const;
};
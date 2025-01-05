#include "question.hpp"
#include <iostream>
#include <algorithm>

Question::Question()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    generateQuestion();
    generateOptions();
}

// Getters
std::string Question::getQuestionText() const
{
    return text;
}


// Metoda do generowania losowego działania matematycznego
void Question::generateQuestion()
{
    operand1 = std::rand() % 101; // 0-100
    operand2 = std::rand() % 101; // 0-100
    operation = getRandomOperation();

    // Zapewnienie, że wynik odejmowania nie będzie ujemny
    if (operation == '-' && operand1 < operand2)
    {
        std::swap(operand1, operand2);
    }

    // Tworzenie tekstu pytania
    text = std::to_string(operand1) + " " + operation + " " + std::to_string(operand2) + " = ?";
}

// Metoda pomocnicza do wyboru losowego operatora
char Question::getRandomOperation() const
{
    int op = std::rand() % 3; // 0,1,2
    switch(op)
    {
        case 0: return '+';
        case 1: return '-';
        case 2: return '*';
        default: return '+';
    }
}

// Metoda do generowania opcji odpowiedzi
void Question::generateOptions()
{
    int correct;
    switch(operation)
    {
        case '+': correct = operand1 + operand2; break;
        case '-': correct = operand1 - operand2; break;
        case '*': correct = operand1 * operand2; break;
        default: correct = 0;
    }
    correctAnswer = correct;

    // Tworzenie opcji odpowiedzi

}

int Question::operator()()
{
    std::cout << text << std::endl;
    return correctAnswer;
}
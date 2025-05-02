#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>

using namespace std;

const int MAX_DIGITS = 105;     // Support for numbers up to 100 digits + sign + buffer
const int MAX_EXPR_LEN = 2000;  // Maximum expression length
const int MAX_TOKENS = 2000;    // Maximum number of tokens

// BigInt Class with Limb-Based Operations
class BigInt {
private:
    std::vector<unsigned long long> limbs; // Little-endian: limbs[0] is least significant
    bool isNegative;
    const unsigned long long BASE = 1000000000000000000ULL; // 10^18

public:
    // Constructor from string
    BigInt(const std::string& num = "0") {
        isNegative = (num[0] == '-');
        std::string digits = isNegative ? num.substr(1) : num;
        limbs = stringToLimbs(digits);
    }

    // Copy constructor
    BigInt(const BigInt& other) : limbs(other.limbs), isNegative(other.isNegative) {}

    // Assignment operator
    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            limbs = other.limbs;
            isNegative = other.isNegative;
        }
        return *this;
    }

    // Convert back to string
    std::string toString() const {
        if (limbs.empty() || (limbs.size() == 1 && limbs[0] == 0)) {
            return "0";
        }
        std::string result;
        for (int i = limbs.size() - 1; i >= 0; --i) {
            std::string limbStr = std::to_string(limbs[i]);
            if (i < limbs.size() - 1) {
                limbStr = std::string(18 - limbStr.length(), '0') + limbStr; // Pad with zeros
            }
            result += limbStr;
        }
        while (result.length() > 1 && result[0] == '0') result.erase(0, 1); // Remove leading zeros
        return isNegative ? "-" + result : result;
    }

    // Addition
    BigInt add(const BigInt& other) const {
        BigInt result;
        if (isNegative == other.isNegative) {
            result.limbs = addLimbs(limbs, other.limbs);
            result.isNegative = isNegative;
        } else {
            int cmp = compareLimbs(limbs, other.limbs);
            if (cmp >= 0) {
                result.limbs = subtractLimbs(limbs, other.limbs);
                result.isNegative = isNegative;
            } else {
                result.limbs = subtractLimbs(other.limbs, limbs);
                result.isNegative = other.isNegative;
            }
        }
        return result;
    }

    // Subtraction
    BigInt subtract(const BigInt& other) const {
        BigInt result;
        if (isNegative == other.isNegative) {
            int cmp = compareLimbs(limbs, other.limbs);
            if (cmp >= 0) {
                result.limbs = subtractLimbs(limbs, other.limbs);
                result.isNegative = isNegative;
            } else {
                result.limbs = subtractLimbs(other.limbs, limbs);
                result.isNegative = !isNegative;
            }
        } else {
            result.limbs = addLimbs(limbs, other.limbs);
            result.isNegative = isNegative;
        }
        return result;
    }

    // Multiplication using Karatsuba algorithm
    BigInt multiply(const BigInt& other) const {
        BigInt result;
        result.isNegative = isNegative != other.isNegative;
        result.limbs = karatsubaMultiply(limbs, other.limbs);
        return result;
    }

    // Division using optimized long division
    BigInt divide(const BigInt& other) const {
        if (other.limbs.size() == 1 && other.limbs[0] == 0) {
            throw std::runtime_error("Error: Division by zero");
        }
        BigInt quotient;
        quotient.isNegative = isNegative != other.isNegative;
        std::vector<unsigned long long> remainder;
        quotient.limbs = optimizedLongDivision(limbs, other.limbs, remainder);
        return quotient;
    }

private:
    // Convert string to limbs
    std::vector<unsigned long long> stringToLimbs(const std::string& digits) const {
        std::vector<unsigned long long> result;
        std::string num = digits;
        while (num.length() > 18) {
            std::string limbStr = num.substr(num.length() - 18);
            num = num.substr(0, num.length() - 18);
            unsigned long long limb = 0;
            for (char digit : limbStr) {
                limb = limb * 10 + (digit - '0');
            }
            result.push_back(limb);
        }
        if (!num.empty()) {
            unsigned long long limb = 0;
            for (char digit : num) {
                limb = limb * 10 + (digit - '0');
            }
            result.push_back(limb);
        }
        while (result.size() > 1 && result.back() == 0) result.pop_back();
        return result;
    }

    // Compare two limb vectors
    int compareLimbs(const std::vector<unsigned long long>& a, const std::vector<unsigned long long>& b) const {
        if (a.size() != b.size()) return a.size() > b.size() ? 1 : -1;
        for (int i = a.size() - 1; i >= 0; --i) {
            if (a[i] != b[i]) return a[i] > b[i] ? 1 : -1;
        }
        return 0;
    }

    // Add two limb vectors
    std::vector<unsigned long long> addLimbs(const std::vector<unsigned long long>& a, const std::vector<unsigned long long>& b) const {
        std::vector<unsigned long long> result;
        unsigned long long carry = 0;
        size_t maxSize = std::max(a.size(), b.size());
        for (size_t i = 0; i < maxSize || carry; ++i) {
            unsigned long long sum = carry;
            if (i < a.size()) sum += a[i];
            if (i < b.size()) sum += b[i];
            carry = sum / BASE;
            result.push_back(sum % BASE);
        }
        return result;
    }

    // Subtract two limb vectors (assumes a >= b)
    std::vector<unsigned long long> subtractLimbs(const std::vector<unsigned long long>& a, const std::vector<unsigned long long>& b) const {
        std::vector<unsigned long long> result;
        unsigned long long borrow = 0;
        for (size_t i = 0; i < a.size(); ++i) {
            unsigned long long diff = a[i] - borrow;
            if (i < b.size()) diff -= b[i];
            borrow = (diff > a[i]) ? 1 : 0;
            result.push_back((diff + BASE) % BASE);
        }
        while (result.size() > 1 && result.back() == 0) result.pop_back();
        return result;
    }

    // Karatsuba multiplication
    std::vector<unsigned long long> karatsubaMultiply(const std::vector<unsigned long long>& a, const std::vector<unsigned long long>& b) const {
        size_t n = std::max(a.size(), b.size());
        std::vector<unsigned long long> a_padded = a;
        std::vector<unsigned long long> b_padded = b;
        a_padded.resize(n, 0);
        b_padded.resize(n, 0);

        // Base case: if either number is single limb, perform direct multiplication
        if (n == 1) {
            std::vector<unsigned long long> result(2, 0);
            __int128 product = (__int128)a_padded[0] * b_padded[0];
            result[0] = product % BASE;
            result[1] = product / BASE;
            while (result.size() > 1 && result.back() == 0) result.pop_back();
            return result;
        }

        size_t k = n / 2;
        std::vector<unsigned long long> a_low(a_padded.begin(), a_padded.begin() + k);
        std::vector<unsigned long long> a_high(a_padded.begin() + k, a_padded.end());
        std::vector<unsigned long long> b_low(b_padded.begin(), b_padded.begin() + k);
        std::vector<unsigned long long> b_high(b_padded.begin() + k, b_padded.end());

        std::vector<unsigned long long> p1 = karatsubaMultiply(a_high, b_high);
        std::vector<unsigned long long> p2 = karatsubaMultiply(a_low, b_low);
        std::vector<unsigned long long> a_sum = addLimbs(a_high, a_low);
        std::vector<unsigned long long> b_sum = addLimbs(b_high, b_low);
        std::vector<unsigned long long> p3 = karatsubaMultiply(a_sum, b_sum);

        std::vector<unsigned long long> temp = subtractLimbs(p3, p1);
        std::vector<unsigned long long> middle = subtractLimbs(temp, p2);

        std::vector<unsigned long long> p1_shifted(p1.size() + 2 * k, 0);
        for (size_t i = 0; i < p1.size(); ++i) {
            p1_shifted[i + 2 * k] = p1[i];
        }
        std::vector<unsigned long long> middle_shifted(middle.size() + k, 0);
        for (size_t i = 0; i < middle.size(); ++i) {
            middle_shifted[i + k] = middle[i];
        }

        std::vector<unsigned long long> result = addLimbs(p1_shifted, middle_shifted);
        result = addLimbs(result, p2);
        while (result.size() > 1 && result.back() == 0) result.pop_back();
        return result;
    }

    // Optimized division for single-limb divisors (common case)
    std::vector<unsigned long long> divideByShort(
        const std::vector<unsigned long long>& a, 
        unsigned long long b, 
        unsigned long long& remainder) const {
        
        std::vector<unsigned long long> result(a.size());
        remainder = 0;
        
        for (int i = a.size() - 1; i >= 0; --i) {
            __int128 current = (__int128)remainder * BASE + a[i];
            result[i] = current / b;
            remainder = current % b;
        }
        
        while (result.size() > 1 && result.back() == 0) result.pop_back();
        return result;
    }

    // Helper: Multiply by a short integer using Karatsuba
    std::vector<unsigned long long> multiplyByShort(
        const std::vector<unsigned long long>& a, 
        unsigned long long b) const {
        
        std::vector<unsigned long long> b_vec = {b};
        return karatsubaMultiply(a, b_vec);
    }

    // Helper function for better quotient digit estimation
    __int128 estimateQuotientDigit(
        const std::vector<unsigned long long>& partial, 
        const std::vector<unsigned long long>& divisor) const {
        
        if (partial.size() <= divisor.size()) {
            if (compareLimbs(partial, divisor) < 0) return 0;
            if (partial.size() < divisor.size()) return 1;
        }
        
        // Use 2-3 most significant limbs for better estimation
        __int128 dividend_high;
        if (partial.size() >= 2) {
            dividend_high = ((__int128)partial[partial.size()-1] * BASE + 
                          partial[partial.size()-2]);
        } else {
            dividend_high = partial.back();
        }
        
        __int128 divisor_high = divisor.back();
        __int128 q = dividend_high / divisor_high;
        
        // Cap the quotient to avoid overflow
        if (q >= BASE) q = BASE - 1;
        
        return q;
    }

    // Multiply by a single digit and subtract in one pass for efficiency
    std::vector<unsigned long long> multiplyAndSubtract(
        const std::vector<unsigned long long>& a,
        const std::vector<unsigned long long>& b,
        unsigned long long q,
        bool& overflow) const {
        
        std::vector<unsigned long long> result(a);
        __int128 borrow = 0;
        
        for (size_t i = 0; i < b.size(); ++i) {
            __int128 product = (__int128)b[i] * q + borrow;
            __int128 diff = result[i] - (product % BASE);
            borrow = product / BASE;
            
            if (diff < 0) {
                diff += BASE;
                borrow += 1;
            }
            
            result[i] = diff;
        }
        
        // Handle overflow in higher positions
        for (size_t i = b.size(); i < result.size() && borrow > 0; ++i) {
            __int128 diff = result[i] - borrow;
            if (diff < 0) {
                diff += BASE;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result[i] = diff;
        }
        
        // Check for overflow
        overflow = (borrow > 0);
        
        // Remove leading zeros
        while (result.size() > 1 && result.back() == 0) result.pop_back();
        
        return result;
    }

    // Improved long division algorithm
    std::vector<unsigned long long> optimizedLongDivision(
        const std::vector<unsigned long long>& dividend, 
        const std::vector<unsigned long long>& divisor, 
        std::vector<unsigned long long>& remainder) const {
        
        // Special case: division by a single limb
        if (divisor.size() == 1) {
            unsigned long long rem = 0;
            auto result = divideByShort(dividend, divisor[0], rem);
            remainder = {rem};  // Convert remainder to vector
            return result;
        }
        
        // Check if dividend < divisor
        size_t m = dividend.size();
        size_t n = divisor.size();
        if (m < n || (m == n && compareLimbs(dividend, divisor) < 0)) {
            remainder = dividend;
            return {0};
        }
        
        // Normalize divisor (scale up to make the leading digit close to BASE)
        unsigned long long d = BASE / (divisor.back() + 1);
        std::vector<unsigned long long> normalizedDividend = (d == 1) ? 
            dividend : multiplyByShort(dividend, d);
        std::vector<unsigned long long> normalizedDivisor = (d == 1) ? 
            divisor : multiplyByShort(divisor, d);
            
        std::vector<unsigned long long> quotient(m - n + 1, 0);
        std::vector<unsigned long long> partialDividend;
        
        // Copy initial portion of dividend to partial
        for (size_t i = m - n; i < m; i++) {
            partialDividend.push_back(normalizedDividend[i]);
        }
        
        for (int j = m - n; j >= 0; --j) {
            // Bring down next digit
            if (j > 0) {
                partialDividend.insert(partialDividend.begin(), normalizedDividend[j-1]);
            }
            
            // Estimate quotient digit
            __int128 qEstimate = estimateQuotientDigit(partialDividend, normalizedDivisor);
            
            // Multiply and subtract in one step with overflow handling
            bool overflow = false;
            auto product = multiplyAndSubtract(partialDividend, normalizedDivisor, qEstimate, overflow);
            
            // Adjust quotient if needed
            if (overflow) {
                qEstimate--;
                // Add back the divisor once
                partialDividend = addLimbs(product, normalizedDivisor);
            } else {
                partialDividend = product;
            }
            
            quotient[j] = qEstimate;
        }
        
        // Denormalize remainder if needed
        unsigned long long dummy = 0;
        remainder = (d == 1) ? partialDividend : divideByShort(partialDividend, d, dummy);
        
        // Remove leading zeros
        while (quotient.size() > 1 && quotient.back() == 0) quotient.pop_back();
        return quotient;
    }
};

// Stack Implementation for BigInt
struct BigIntStack {
    BigInt items[MAX_TOKENS];
    int top = -1;

    bool push(const BigInt& item) {
        if (top >= MAX_TOKENS - 1) return false;
        items[++top] = item;
        return true;
    }

    bool pop(BigInt& item) {
        if (top < 0) return false;
        item = items[top--];
        return true;
    }

    BigInt& peek() {
        return items[top];
    }

    bool isEmpty() {
        return top < 0;
    }
};

// Stack Implementation for Operators (infix to postfix)
struct OperatorStack {
    char items[MAX_TOKENS][MAX_DIGITS];
    int top = -1;

    bool push(const char* item) {
        if (top >= MAX_TOKENS - 1) return false;
        strcpy(items[++top], item);
        return true;
    }

    bool pop(char* item = nullptr) {
        if (top < 0) return false;
        if (item) strcpy(item, items[top]);
        top--;
        return true;
    }

    const char* peek() {
        return top >= 0 ? items[top] : nullptr;
    }

    bool isEmpty() {
        return top < 0;
    }
};

// Token Structure
struct Token {
    char value[MAX_DIGITS];
};

// Utility Functions
bool isValidNumber(const char* num) {
    if (num[0] == '\0' || (num[0] == '-' && num[1] == '\0')) return false;
    int i = (num[0] == '-') ? 1 : 0;
    for (; num[i]; i++) {
        if (!isdigit(num[i])) return false;
    }
    return true;
}

// Expression Processing

// Tokenizes an expression, handling unary minus correctly
int tokenize(char* expr, Token tokens[]) {
    int count = 0, i = 0, j;
    bool expectOperand = true; // True at start or after operator/(
    
    while (expr[i]) {
        if (expr[i] == ' ') {
            i++;
            continue;
        }
        
        // Handle operators and parentheses
        if (expr[i] == '+' || expr[i] == '*' || expr[i] == '/' || expr[i] == '(' || expr[i] == ')') {
            tokens[count].value[0] = expr[i];
            tokens[count].value[1] = '\0';
            expectOperand = (expr[i] != ')');
            count++;
            i++;
            continue;
        }
        
        // Handle minus (as operator or negative sign)
        if (expr[i] == '-') {
            if (expectOperand) {  // Unary minus (part of number)
                j = 0;
                tokens[count].value[j++] = expr[i++];
                while (isdigit(expr[i])) {
                    if (j >= MAX_DIGITS - 1) return count;
                    tokens[count].value[j++] = expr[i++];
                }
                tokens[count].value[j] = '\0';
                expectOperand = false;
            } else {  // Binary minus (operator)
                tokens[count].value[0] = expr[i];
                tokens[count].value[1] = '\0';
                expectOperand = true;
                i++;
            }
            count++;
            continue;
        }
        
        // Handle numbers
        if (isdigit(expr[i])) {
            j = 0;
            while (isdigit(expr[i])) {
                if (j >= MAX_DIGITS - 1) return count;
                tokens[count].value[j++] = expr[i++];
            }
            tokens[count].value[j] = '\0';
            expectOperand = false;
            count++;
            continue;
        }
        
        // Invalid character
        return -1;
    }
    
    return count;
}

bool isNumber(const char* token) {
    return isValidNumber(token);
}

int precedence(const char* op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 1;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) return 2;
    return 0;
}

bool isValidExpression(Token infix[], int count) {
    if (count == 0) return false;
    
    int parenCount = 0;
    bool expectOperand = true;
    
    for (int i = 0; i < count; i++) {
        char* token = infix[i].value;
        
        if (isNumber(token)) {
            if (!expectOperand) return false;  // Number following non-operator
            expectOperand = false;
        } 
        else if (strcmp(token, "(") == 0) {
            parenCount++;
            expectOperand = true;
        } 
        else if (strcmp(token, ")") == 0) {
            if (expectOperand || parenCount == 0) return false;
            parenCount--;
            expectOperand = false;
        } 
        else if (strcmp(token, "+") == 0 || strcmp(token, "-") == 0 ||
                strcmp(token, "*") == 0 || strcmp(token, "/") == 0) {
            if (expectOperand) return false;  // Operator following operator
            expectOperand = true;
        } 
        else {
            return false;  // Invalid token
        }
    }
    
    return parenCount == 0 && !expectOperand;  // Check balanced parentheses and proper ending
}

int infixToPostfix(Token infix[], int count, Token postfix[]) {
    OperatorStack stack;
    int j = 0;

    for (int i = 0; i < count; i++) {
        char* token = infix[i].value;

        if (isNumber(token)) {
            strcpy(postfix[j++].value, token);
        } 
        else if (strcmp(token, "(") == 0) {
            if (!stack.push(token)) return -1;  // Stack overflow
        } 
        else if (strcmp(token, ")") == 0) {
            while (!stack.isEmpty() && strcmp(stack.peek(), "(") != 0) {
                strcpy(postfix[j++].value, stack.peek());
                stack.pop();
            }
            if (stack.isEmpty()) return -1;  // Mismatched parentheses
            stack.pop();  // Discard the '('
        } 
        else {  // Operator
            while (!stack.isEmpty() && strcmp(stack.peek(), "(") != 0 &&
                   precedence(stack.peek()) >= precedence(token)) {
                strcpy(postfix[j++].value, stack.peek());
                stack.pop();
            }
            if (!stack.push(token)) return -1;  // Stack overflow
        }
    }

    while (!stack.isEmpty()) {
        if (strcmp(stack.peek(), "(") == 0) return -1;  // Mismatched parentheses
        strcpy(postfix[j++].value, stack.peek());
        stack.pop();
    }

    return j;
}

void evaluatePostfix(Token postfix[], int count, char* result) {
    BigIntStack stack;

    for (int i = 0; i < count; i++) {
        char* token = postfix[i].value;

        if (isNumber(token)) {
            BigInt num(token);
            if (!stack.push(num)) {
                strcpy(result, "Error: Stack overflow");
                return;
            }
        } 
        else {  // Operator
            if (stack.isEmpty()) {
                strcpy(result, "Error: Invalid expression");
                return;
            }
            
            BigInt b, a;
            if (!stack.pop(b)) {
                strcpy(result, "Error: Stack underflow");
                return;
            }
            if (stack.isEmpty()) {
                strcpy(result, "Error: Invalid expression");
                return;
            }
            if (!stack.pop(a)) {
                strcpy(result, "Error: Stack underflow");
                return;
            }
            
            BigInt tempResult;
            if (strcmp(token, "+") == 0) {
                tempResult = a.add(b);
            } else if (strcmp(token, "-") == 0) {
                tempResult = a.subtract(b);
            } else if (strcmp(token, "*") == 0) {
                tempResult = a.multiply(b);
            } else if (strcmp(token, "/") == 0) {
                try {
                    tempResult = a.divide(b);
                } catch (const std::runtime_error& e) {
                    strcpy(result, e.what());
                    return;
                }
            } else {
                strcpy(result, "Error: Unknown operator");
                return;
            }
            
            if (!stack.push(tempResult)) {
                strcpy(result, "Error: Stack overflow");
                return;
            }
        }
    }
    
    // Get final result
    if (stack.isEmpty()) {
        strcpy(result, "Error: Invalid expression");
        return;
    }
    
    BigInt finalResult;
    if (!stack.pop(finalResult)) {
        strcpy(result, "Error: Stack underflow");
        return;
    }
    
    // Should have exactly one value left in stack
    if (!stack.isEmpty()) {
        strcpy(result, "Error: Invalid expression");
        return;
    }
    
    std::string resStr = finalResult.toString();
    if (resStr.length() >= MAX_DIGITS) {
        strcpy(result, "Error: Result too large");
    } else {
        strcpy(result, resStr.c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " input_file output_file" << endl;
        return 1;
    }
    
    ifstream inFile(argv[1]);
    ofstream outFile(argv[2]);
    
    if (!inFile || !outFile) {
        cout << "Error opening files" << endl;
        return 1;
    }
    
    char expr[MAX_EXPR_LEN];
    
    while (inFile.getline(expr, MAX_EXPR_LEN)) {
        if (strlen(expr) >= MAX_EXPR_LEN - 1) {
            cout << "Error: Expression too long" << endl;
            outFile << "Error: Expression too long" << endl;
            continue;
        }
        
        Token infix[MAX_TOKENS], postfix[MAX_TOKENS];
        char result[MAX_DIGITS];
        
        // Tokenize expression
        int tokenCount = tokenize(expr, infix);
        
        if (tokenCount < 0 || !isValidExpression(infix, tokenCount)) {
            strcpy(result, "Error: Invalid expression");
            cout << result << endl;
            outFile << result << endl;
            continue;
        }
        
        // Convert infix to postfix
        int postfixCount = infixToPostfix(infix, tokenCount, postfix);
        
        if (postfixCount < 0) {
            strcpy(result, "Error: Invalid expression");
            cout << result << endl;
            outFile << result << endl;
            continue;
        }
        
        // Evaluate postfix expression
        evaluatePostfix(postfix, postfixCount, result);
        
        cout << result << endl;
        outFile << result << endl;
    }
    
    inFile.close();
    outFile.close();
    
    return 0;
}
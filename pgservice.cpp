#include <bits/stdc++.h>
using namespace std;

/*
    Payment Gateway Service (LLD)
    ------------------------------
    - A payment gateway service is responsible for processing online payments securely and efficiently. 
      It acts as an intermediary between the merchant's website and the financial institutions involved in the transaction.
    - Key Components:
        1. Payment Processor: Handles the actual processing of payments, including authorization, capture, and settlement.
        2. Fraud Detection System: Monitors transactions for suspicious activity and helps prevent fraudulent transactions.
        3. API Gateway: Provides a secure interface for merchants to integrate with the payment gateway and manage their transactions.
        4. Database: Stores transaction records, merchant information, and other relevant data securely.

    Step1: Write Behavior, not classes
    - Key Features (Behaviors):
        1. Receive payment request
        2. Validate request
        3. Runs fraud check
        4. Authorize with processor
        5. Persist transaction
        6. Return payment status
        7. Emit logs/events

    Step2: Define core domain object first: design artifacts should be noun, not service classes
    - Entities
        1. Transaction
        2. PaymentMethod
        3. Merchant
        4. MonetoryData
        5. TransactionStatus
        6. ErrorCodes

    Step3: Write responsibilities before method names
    - For each component: What it owns, What it does, What it must never do
        1. PGService: owns orchestration
        2. PaymentProcessor: talks to externalGateway, authorize/capture/refund
        3. FraudDetector: returns risk decision
        4. TransactionRepository: owns persistence of DB
        5. PaymentStatus: talks about the final status of the payment

    Step4: Create skeleton in "dependency direction" 
        1. Enums & values objects
        2. Domain entities
        3. Interfaces for external systems
        4. Concrete adaptors (DB, gateway, fraud)
        5. Orchestrator service
        6. API/Controller layer
        7. Main

    Step5: Use Interface-First design
    - Before implementation, define interfaces for things that can change:
        1. IPaymentProcessor
        2. IFraudDetector
        3. ITransactionRepository
        4. ILogger

    Step6: Define state machine early
    - Possible transitions:
        1. CREATED -> FRAUD_CHECK
        2. FRAUD_CHECK -> AUTHORIZED/REJECTED
        3. AUTHORIZED -> CAPTURED
        4. CAPTURED ->SETTLED
        5. -> FAILED (with any reason)

    Step7: Decide non-functional rules up front
        1. Idempotancy rule (same request retried twice)
        2. Timeouts & retries for external calls
        3. Consistency model (when DB update fails after auth)
        4. Observality (what to log/metric)
        5. Security constraints (PII, tokentization)

    Step8: Implement vertically in thin slices
    - Don't build all classes fully first, build one full journey e2e
        1. Create transaction
        2. Fraud pass
        3. Authorize
        4. Save transaction
        5. Return response
        -------
        6. Real fraud logic
        7. Real DB adapter
        8. Retry/error handling
        9. Metrics/logging

    Step9: Testing strategy while designing
    - Test each layer
        1. Domain tests: status transitions, amount rules
        2. Service test: orchestration success/failure paths
        3. Adapter tests: DB/gateway integration
        4. e2e happy path + retry path
        Note: If we can't test a class in isolation, design is probably too coupled

    Step10: Practical checklist
        1. Single responsibilty per class?
        2. No constructor side effects?
        3. Ownership/lifetime clear?
        4. External dependencies behind interfaces?
        5. State transitions explicit & guarded?
        6. Error model consistent?
        7. Logs/metrics/audit included?
        8. Retries/idempotency defined?
        9. Easy to unit test?
        10. Easy to add another PSP without changing core flow?

*/

/*
LLD demo implementing the 10-step approach:
    1) behavior flow
    2) domain objects
    3) clear responsibilities
    4) skeleton order
    5) interface-first
    6) state transitions
    7) non-functional rules
    8) thin slice
    9) testable seams
    10) checklist.

This is intentionally small and print-driven for learning.
*/

enum class TransactionStatus {
    CREATED,
    FRAUD_CHECKED,
    AUTHORIZED,
    FAILED
};

struct Transaction {
    std::string id;
    double amount;
    std::string currency;
    TransactionStatus status;

    Transaction(const std::string &idIn, double amountIn, const std::string &currencyIn)
        : id(idIn), amount(amountIn), currency(currencyIn), status(TransactionStatus::CREATED) {}
};

class IFraudDetector {
 public:
    virtual ~IFraudDetector() {}
    virtual bool isAllowed(const Transaction &txn) = 0;
};

class IPaymentProcessor {
 public:
    virtual ~IPaymentProcessor() {}
    virtual bool authorize(const Transaction &txn) = 0;
};

class ITransactionRepository {
 public:
    virtual ~ITransactionRepository() {}
    virtual void save(const Transaction &txn) = 0;
};

class SimpleFraudDetector : public IFraudDetector {
 public:
    bool isAllowed(const Transaction &txn) override {
        std::cout << "[Fraud] checking txn=" << txn.id << ", amount=" << txn.amount << '\n';
        return txn.amount <= 1000.0;
    }
};

class SimplePaymentProcessor : public IPaymentProcessor {
 public:
    bool authorize(const Transaction &txn) override {
        std::cout << "[Processor] authorizing txn=" << txn.id << " via mock gateway\n";
        return txn.amount > 0.0;
    }
};

class InMemoryTransactionRepository : public ITransactionRepository {
 public:
    void save(const Transaction &txn) override {
        std::cout << "[Repository] saved txn=" << txn.id << ", status=";
        if (txn.status == TransactionStatus::AUTHORIZED) {
            std::cout << "AUTHORIZED";
        } else if (txn.status == TransactionStatus::FAILED) {
            std::cout << "FAILED";
        } else if (txn.status == TransactionStatus::FRAUD_CHECKED) {
            std::cout << "FRAUD_CHECKED";
        } else {
            std::cout << "CREATED";
        }
        std::cout << '\n';
    }
};

class PGService {
 private:
    IFraudDetector &fraudDetector;
    IPaymentProcessor &paymentProcessor;
    ITransactionRepository &repository;
    std::unordered_set<std::string> seenRequestIds;

 public:
    PGService(
        IFraudDetector &fraudDetectorIn,
        IPaymentProcessor &paymentProcessorIn,
        ITransactionRepository &repositoryIn)
        : fraudDetector(fraudDetectorIn), paymentProcessor(paymentProcessorIn), repository(repositoryIn) {}

    // Function 1: tiny end-to-end thin slice showing what happens in the system.
    bool processPayment(Transaction &txn) {
        std::cout << "\n[PGService] start flow for txn=" << txn.id << "\n";
        std::cout << "[Flow] receive -> validate -> fraud -> authorize -> persist -> respond\n";

        if (seenRequestIds.find(txn.id) != seenRequestIds.end()) {
            std::cout << "[Idempotency] duplicate request, skip re-processing for txn=" << txn.id << '\n';
            return true;
        }

        if (txn.amount <= 0.0 || txn.currency.empty()) {
            txn.status = TransactionStatus::FAILED;
            repository.save(txn);
            std::cout << "[Response] failed: invalid amount/currency\n";
            return false;
        }

        bool fraudAllowed = fraudDetector.isAllowed(txn);
        if (!fraudAllowed) {
            txn.status = TransactionStatus::FAILED;
            repository.save(txn);
            std::cout << "[Response] failed: fraud blocked\n";
            return false;
        }
        txn.status = TransactionStatus::FRAUD_CHECKED;

        bool authorized = paymentProcessor.authorize(txn);
        if (!authorized) {
            txn.status = TransactionStatus::FAILED;
            repository.save(txn);
            std::cout << "[Response] failed: authorization denied\n";
            return false;
        }

        txn.status = TransactionStatus::AUTHORIZED;
        repository.save(txn);
        seenRequestIds.insert(txn.id);
        std::cout << "[Response] success: payment authorized\n";
        return true;
    }

    // Function 2: prints non-functional/design checklist early, as part of LLD thinking.
    void printDesignChecklist() const {
        std::cout << "\n[Checklist]"
                  << " idempotency=yes, retries=not-implemented,"
                  << " timeout=not-implemented, logs=print-based,"
                  << " interfaces=testable-seams\n";
    }
};

int main() {
    SimpleFraudDetector fraud;
    SimplePaymentProcessor processor;
    InMemoryTransactionRepository repo;
    PGService service(fraud, processor, repo);

    service.printDesignChecklist();

    Transaction t1("req-101", 100.50, "USD");
    service.processPayment(t1);

    // Same id demonstrates idempotency behavior.
    Transaction t2("req-101", 100.50, "USD");
    service.processPayment(t2);

    return 0;
}
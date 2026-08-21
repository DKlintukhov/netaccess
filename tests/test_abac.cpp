#include <QtTest>

#include <netaccess/domain.h>
#include <netaccess/protocol.h>

class TestAbac : public QObject
{
    Q_OBJECT

private slots:
    void testPolicyMatchesRole();
    void testPolicyMatchesDepartment();
    void testPolicyMatchesClearance();
    void testPolicyMatchesResourceType();
    void testPolicyMatchesSubjectId();
    void testPolicyMatchesResourceId();
    void testPolicyMatchesWildcard();
    void testEvaluateAllow();
    void testEvaluateDenyByDefault();
    void testEvaluateInactiveSubject();
    void testEvaluateHighestPriority();
    void testEvaluateActionMismatch();
};

void TestAbac::testPolicyMatchesRole()
{
    domain::Subject subject;
    subject.role = domain::Role::admin;

    domain::Resource resource;
    resource.type = domain::ResourceType::file_share;

    domain::Policy policy;
    policy.role_required = domain::Role::admin;
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.role_required = domain::Role::user;
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesDepartment()
{
    domain::Subject subject;
    subject.department = "IT";

    domain::Resource resource;

    domain::Policy policy;
    policy.department_required = "IT";
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.department_required = "HR";
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesClearance()
{
    domain::Subject subject;
    subject.clearance_level = 3;

    domain::Resource resource;

    domain::Policy policy;
    policy.min_clearance = 2;
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.min_clearance = 4;
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesResourceType()
{
    domain::Subject subject;

    domain::Resource resource;
    resource.type = domain::ResourceType::database;

    domain::Policy policy;
    policy.resource_type = domain::ResourceType::database;
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.resource_type = domain::ResourceType::printer;
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesSubjectId()
{
    domain::Subject subject;
    subject.id = 42;

    domain::Resource resource;

    domain::Policy policy;
    policy.subject_id = 42;
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.subject_id = 99;
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesResourceId()
{
    domain::Subject subject;

    domain::Resource resource;
    resource.id = 10;

    domain::Policy policy;
    policy.resource_id = 10;
    QVERIFY(domain::policy_matches(subject, resource, policy));

    policy.resource_id = 20;
    QVERIFY(!domain::policy_matches(subject, resource, policy));
}

void TestAbac::testPolicyMatchesWildcard()
{
    domain::Subject subject;
    domain::Resource resource;

    domain::Policy policy;
    QVERIFY(domain::policy_matches(subject, resource, policy));
}

void TestAbac::testEvaluateAllow()
{
    domain::Subject subject;
    subject.id = 1;
    subject.is_active = true;
    subject.role = domain::Role::user;

    domain::Resource resource;
    resource.id = 10;
    resource.type = domain::ResourceType::file_share;

    domain::Policy policy;
    policy.name = "user-read-file";
    policy.action = domain::Action::read;
    policy.enabled = true;
    policy.priority = 1;
    policy.role_required = domain::Role::user;
    policy.resource_type = domain::ResourceType::file_share;

    std::vector<domain::Policy> policies = {policy};

    auto decision = domain::evaluate_policy(subject, resource, domain::Action::read, policies);
    QVERIFY(decision.allowed);
    QVERIFY(decision.reason.find("user-read-file") != std::string::npos);
}

void TestAbac::testEvaluateDenyByDefault()
{
    domain::Subject subject;
    subject.is_active = true;

    domain::Resource resource;

    std::vector<domain::Policy> policies;

    auto decision = domain::evaluate_policy(subject, resource, domain::Action::read, policies);
    QVERIFY(!decision.allowed);
    QCOMPARE(decision.reason, std::string("no_matching_policy"));
}

void TestAbac::testEvaluateInactiveSubject()
{
    domain::Subject subject;
    subject.is_active = false;

    domain::Resource resource;

    domain::Policy policy;
    policy.enabled = true;
    policy.action = domain::Action::read;

    std::vector<domain::Policy> policies = {policy};

    auto decision = domain::evaluate_policy(subject, resource, domain::Action::read, policies);
    QVERIFY(!decision.allowed);
    QCOMPARE(decision.reason, std::string("account_inactive"));
}

void TestAbac::testEvaluateHighestPriority()
{
    domain::Subject subject;
    subject.id = 1;
    subject.is_active = true;
    subject.role = domain::Role::admin;

    domain::Resource resource;
    resource.type = domain::ResourceType::file_share;

    domain::Policy low;
    low.name = "low";
    low.action = domain::Action::read;
    low.enabled = true;
    low.priority = 1;

    domain::Policy high;
    high.name = "high";
    high.action = domain::Action::read;
    high.enabled = true;
    high.priority = 10;

    std::vector<domain::Policy> policies = {low, high};

    auto decision = domain::evaluate_policy(subject, resource, domain::Action::read, policies);
    QVERIFY(decision.allowed);
    QVERIFY(decision.reason.find("high") != std::string::npos);
}

void TestAbac::testEvaluateActionMismatch()
{
    domain::Subject subject;
    subject.is_active = true;

    domain::Resource resource;

    domain::Policy policy;
    policy.name = "write-only";
    policy.action = domain::Action::write;
    policy.enabled = true;

    std::vector<domain::Policy> policies = {policy};

    auto decision = domain::evaluate_policy(subject, resource, domain::Action::read, policies);
    QVERIFY(!decision.allowed);
}

// ---------------------------------------------------------------------------
// Protocol tests
// ---------------------------------------------------------------------------

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    void testOpRoundtrip();
    void testStatusRoundtrip();
    void testResultCodeRoundtrip();
    void testRequestSerialization();
    void testRequestDeserialization();
    void testResponseSerialization();
    void testResponseDeserialization();
    void testFrameRoundtrip();
    void testFrameTooLarge();
};

void TestProtocol::testOpRoundtrip()
{
    const protocol::Op ops[] = {
        protocol::Op::AUTH,
        protocol::Op::LOGOUT,
        protocol::Op::ME,
        protocol::Op::RESOURCE_LIST,
        protocol::Op::RESOURCE_GET,
        protocol::Op::RESOURCE_CREATE,
        protocol::Op::RESOURCE_UPDATE,
        protocol::Op::RESOURCE_DELETE,
        protocol::Op::POLICY_LIST,
        protocol::Op::POLICY_CREATE,
        protocol::Op::POLICY_UPDATE,
        protocol::Op::POLICY_DELETE,
        protocol::Op::USER_LIST,
        protocol::Op::USER_CREATE,
        protocol::Op::USER_UPDATE,
        protocol::Op::USER_DELETE,
        protocol::Op::ACCESS_CHECK,
        protocol::Op::GRANT_ACCESS,
        protocol::Op::REVOKE_ACCESS,
        protocol::Op::AUDIT_QUERY,
    };

    for (const auto op : ops)
    {
        const char* str = protocol::opToString(op);
        QVERIFY(str != nullptr);
        auto roundtrip = protocol::opFromString(QString::fromUtf8(str));
        QVERIFY(roundtrip.has_value());
        QCOMPARE(*roundtrip, op);
    }
}

void TestProtocol::testStatusRoundtrip()
{
    QCOMPARE(QString::fromUtf8(protocol::statusToString(protocol::Status::ok)), QStringLiteral("ok"));
    QCOMPARE(protocol::statusFromString(QStringLiteral("ok")), protocol::Status::ok);
    QVERIFY(!protocol::statusFromString(QStringLiteral("unknown")).has_value());
}

void TestProtocol::testResultCodeRoundtrip()
{
    QCOMPARE(QString::fromUtf8(protocol::resultCodeToString(protocol::ResultCode::OK)), QStringLiteral("OK"));
    QCOMPARE(protocol::resultCodeFromString(QStringLiteral("TOKEN_EXPIRED")), protocol::ResultCode::TOKEN_EXPIRED);
    QVERIFY(!protocol::resultCodeFromString(QStringLiteral("NOPE")).has_value());
}

void TestProtocol::testRequestSerialization()
{
    protocol::Request req;
    req.op = protocol::Op::AUTH;
    req.req_id = 42;
    req.data = {{"username", "ivanov"}, {"password", "secret"}};

    auto json = req.toJson();
    QCOMPARE(json["op"].toString(), QStringLiteral("AUTH"));
    QCOMPARE(json["req_id"].toInt(), 42);

    auto roundtrip = protocol::Request::fromJson(json);
    QVERIFY(roundtrip.has_value());
    QCOMPARE(roundtrip->op, protocol::Op::AUTH);
    QCOMPARE(roundtrip->req_id, 42);
}

void TestProtocol::testRequestDeserialization()
{
    QJsonObject bad;
    bad["op"] = "AUTH";
    QVERIFY(!protocol::Request::fromJson(bad).has_value());

    QJsonObject unknown;
    unknown["op"] = "UNKNOWN_OP";
    unknown["req_id"] = 1;
    QVERIFY(!protocol::Request::fromJson(unknown).has_value());
}

void TestProtocol::testResponseSerialization()
{
    protocol::Response resp;
    resp.op = protocol::Op::AUTH;
    resp.req_id = 42;
    resp.status = protocol::Status::ok;
    resp.code = protocol::ResultCode::OK;

    auto json = resp.toJson();
    QCOMPARE(json["status"].toString(), QStringLiteral("ok"));

    auto roundtrip = protocol::Response::fromJson(json);
    QVERIFY(roundtrip.has_value());
    QCOMPARE(roundtrip->status, protocol::Status::ok);
}

void TestProtocol::testResponseDeserialization()
{
    QJsonObject bad;
    bad["op"] = "AUTH";
    bad["status"] = "ok";
    QVERIFY(!protocol::Response::fromJson(bad).has_value());
}

void TestProtocol::testFrameRoundtrip()
{
    QJsonObject obj;
    obj["key"] = "value";
    obj["number"] = 42;

    QByteArray framed = protocol::frame(obj);

    QBuffer buf(&framed);
    buf.open(QIODevice::ReadOnly);

    auto result = protocol::readFrame(&buf);
    QVERIFY(result.has_value());
    QCOMPARE(result->value("key").toString(), QStringLiteral("value"));
    QCOMPARE(result->value("number").toInt(), 42);
}

void TestProtocol::testFrameTooLarge()
{
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(protocol::kMaxPayloadSize + 1);

    QByteArray payload = QByteArray(1024, 'x');
    QByteArray data = header + payload;

    QBuffer buf(&data);
    buf.open(QIODevice::ReadOnly);

    auto result = protocol::readFrame(&buf);
    QVERIFY(!result.has_value());
}

int main(int argc, char* argv[])
{
    int status = 0;
    {
        TestAbac tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestProtocol tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_abac.moc"

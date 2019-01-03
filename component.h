#pragma once

class Node
{
public:
    virtual ~Node();
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QVariantMap config();
    virtual void onConnected();

    QString topic() const;
    void update(const QByteArray &payload);
protected:
    Node(const QString &id);
private:
    QString nodeId;
};


ConnectedNode : public Node
{
    QString id() const override {
        return "connected";
    }
    QString name() const override {
        return "Connected";
    }
    QVariantMap config() override {
    }

};

ActiveNode: public Node
{
};


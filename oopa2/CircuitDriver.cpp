#include "CircuitDriver.h"
#include "Node.h"
//#include <vld.h>
#include <iostream>
#include <algorithm>

CircuitDriver::CircuitDriver() : reader_("") {
}

CircuitDriver::CircuitDriver(const std::string path) : reader_(path) {
    reader_.ReadFile();
}

CircuitDriver::~CircuitDriver() {
    for (auto eleme_node : nodes_) {
        delete eleme_node;
    }
}

void CircuitDriver::CreateNodes() {
    auto f = [](auto i) -> int {
        if (i.find("HIGH") != std::string::npos)
            return 1;
        else
            return 0;
    };
    auto types = reader_.GetTypes();
    for (auto i = types.begin(); i != types.end(); ++i) {
        const auto second = i->substr(i->find(":")+1, i->find(";") - i->find(":") - 1);
        const auto first = i->substr(0, i->find(":"));
        if (i->find("INPUT") != std::string::npos) {
            nodes_.push_back(NodeFactory::create("INPUT"));
            nodes_.back()->SetValue(f(*i));
        } else {
            nodes_.push_back(NodeFactory::create(second.c_str()));
        }
        nodes_.back()->SetName(first);
    }
    std::vector<Node*> temp_vec;
    int modifier;
    if (reader_.GetAo() > 1) {
        modifier = 0;
    } else {
        modifier = 1;
    }
    auto x = std::vector<Node*>(nodes_.begin(), nodes_.begin() + reader_.GetAi());
    auto y = std::vector<Node*>(nodes_.begin() + reader_.GetAi() + reader_.GetAo()-modifier, nodes_.end());
    auto z = std::vector<Node*>(nodes_.begin() + reader_.GetAi(), nodes_.begin() + reader_.GetAi()-modifier + reader_.GetAo());
    temp_vec.insert(temp_vec.end(), x.begin(), x.end());
    temp_vec.insert(temp_vec.end(), y.begin(), y.end());
    temp_vec.insert(temp_vec.end(), z.begin(), z.end());

    nodes_ = temp_vec;
}

void CircuitDriver::CreateEdges() {
    auto links = reader_.GetLinks();
    auto types = reader_.GetTypes();
    for (const auto l : links) {
        std::string placeholder;
        const auto second = l.substr(l.find(":")+1, l.find(";") - l.find(":"));
        const auto first = l.substr(0, l.find(":"));
        if ((placeholder = second) != "") {
            placeholder.pop_back();
            const auto delimiter = ',';
            size_t pos;
            while ((pos = placeholder.find(delimiter)) != std::string::npos) {
                const auto token = placeholder.substr(0, pos);
                FindValue(nodes_, token)->Attach(FindValue(nodes_, first));
                placeholder.erase(0, pos + 1);
            }
            FindValue(nodes_, placeholder)->Attach(FindValue(nodes_, first));
        }
    }
}

void CircuitDriver::DriveValues() {
    std::for_each(nodes_.begin(), nodes_.end(), [this](Node* node) {
        auto edges = node->GetNodes();
        for (auto e : edges) {
            if (FindValue(nodes_, e->GetName()) != nullptr)
                e->SetValue(FindValue(nodes_, e->GetName())->GetValue());
        }
        node->accept(visitor_);
        std::cout << node->GetName() << ": " << static_cast<int>(node->GetValue()) << std::endl;
        if (node->GetValue() > 1) {
            throw std::invalid_argument("Nodes with feedback detected!Please make sure there is no feedback present in the provided circuit.");
        } 
    });
}

Node* CircuitDriver::FindValue(std::vector<Node*> vec, std::string element) {
    Node *ret;
    auto found = false;
    std::for_each(vec.begin(), vec.end(), [&ret, &element, &found](Node* a) {
        if (a->GetName() == element) {
            ret = a;
            found = true;
        } else if (a->GetName() != element && !found) {
            ret = nullptr;
        }
    });
    return ret;
}

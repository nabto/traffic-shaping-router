#ifndef INPUT_HPP
#define INPUT_HPP

class Input : public Filter, public std::enable_shared_from_this<Input>
{
 public:
    Input() {}
    ~Input() {}
    void handlePacket(std::shared_ptr<ParentPacket> pkt){
        next_->handlePacket(pkt);
    }
};
#endif //INPUT_HPP

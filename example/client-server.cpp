/**
 * @file   main.cpp
 * @author Dennis Sitelew 
 * @date   Jun 08, 2024
 */

#include <cbor/cbor.h>
#include <shp/shp.h>

#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

// Enumerations don't require type ID and treated as plain integers
enum class request_result {
   success,
   error,
};

// Plain (w/o the type ID) structures have to be whitelisted
struct contact {
   std::string name{};
   std::string phone{};
   std::optional<std::string> address{};
};
[[maybe_unused]] consteval void enable_cbor_encoding(contact);

struct phone_book {
   std::vector<contact> contacts{};
};
[[maybe_unused]] consteval void enable_cbor_encoding(phone_book);

namespace add_contact {

// Variants require a type ID when encoding/decoding
inline constexpr int64_t id = 0x01;

struct request {
   std::int64_t id{};
   contact value{};
};

struct response {
   std::int64_t request_id{};
   request_result result{};
   std::optional<std::int64_t> contact_id{};
};
} // namespace add_contact

namespace get_contacts {

// Variants require a type ID when encoding/decoding
inline constexpr int64_t id = 0x02;

struct request {
   std::int64_t id{};
};

struct response {
   std::int64_t request_id{};
   request_result result{};
   std::optional<phone_book> contacts{};
};

} // namespace get_contacts

// Requests can be represented as a variant (requires a type ID)
using request_t = std::variant<add_contact::request, get_contacts::request>;

// Variants require a type ID when encoding/decoding
template <>
struct cbor::type_id<add_contact::request> : std::integral_constant<std::int64_t, add_contact::id> {};

// Variants require a type ID when encoding/decoding
template <>
struct cbor::type_id<get_contacts::request> : std::integral_constant<std::int64_t, get_contacts::id> {};

// Responses can be represented as a variant (requires a type ID)
using response_t = std::variant<add_contact::response, get_contacts::response>;

// Variants require a type ID when encoding/decoding
template <>
struct cbor::type_id<add_contact::response> : std::integral_constant<std::int64_t, add_contact::id> {};

// Variants require a type ID when encoding/decoding
template <>
struct cbor::type_id<get_contacts::response> : std::integral_constant<std::int64_t, get_contacts::id> {};

// Helper function to print byte arrays as one-liners
template <typename T>
auto hex(const T &v) {
   return shp::hex(v, shp::NoOffsets{}, shp::NoNibbleSeparation{}, shp::SingleRow{}, shp::NoASCII{}, shp::UpperCase{});
}

// Dummy "server": it owns a phone book and provides a request-based API for modifying it.
class server {
public:
   // Pretend we are processing a network payload: we get a request message and a buffer for putting a response into it.
   void handle_message(std::span<const std::byte> message, std::vector<std::byte> &response) {
      // Decode a request
      cbor::read_buffer buf{message};
      request_t r{};
      if (auto res = cbor::decode(buf, r)) {
         throw std::system_error{res};
      }

      if (message.size() != buf.read_position()) {
         throw std::runtime_error("Trailing bytes after message");
      }

      // Call an appropriate handler based on the currently active variant alternative
      std::visit([&](const auto &r) { this->handle(r, response); }, r);
   }

private:
   // Handle an "Add Contact" request
   void handle(const add_contact::request &r, std::vector<std::byte> &out) {
      // Simply append a new contact to the current phone book
      const auto count = phone_book_.contacts.size();
      phone_book_.contacts.push_back(r.value);

      // Prepare a response
      out.resize(0);
      cbor::dynamic_buffer buf{out};

      response_t msg{add_contact::response{
         .request_id = r.id,
         .result = request_result::success,
         .contact_id = {count},
      }};

      if (auto res = cbor::encode(buf, msg)) {
         throw std::system_error{res};
      }

      std::cout << "<- Add contact response: " << hex(out) << std::endl;
   }

   // Handle a "Get Contacts" request
   void handle(const get_contacts::request &r, std::vector<std::byte> &out) {
      // Prepare a response
      out.resize(0);
      cbor::dynamic_buffer buf{out};

      response_t msg{get_contacts::response{
         .request_id = r.id,
         .result = request_result::success,
         .contacts = {phone_book_}, // Simply copy the whole phone book
      }};

      if (auto res = cbor::encode(buf, msg)) {
         throw std::system_error{res};
      }

      std::cout << "<- Get contacts response: " << hex(out) << std::endl;
   }

private:
   phone_book phone_book_{};
};

// Dummy "client": it tries to retrieve the phone book from the server using the requests API.
class client {
public:
   explicit client(server &s)
      : server_{&s} {}

   void run() {
      get_contacts();
      add_contact("First Man", "+42 12 32", "On Earth");
      add_contact("John Doe", "+13 25 10");
      add_contact("Mr. Hankey", "+66 613", "North Woods");
      add_contact("Tiny Sal", "-10");
      get_contacts();
   }

private:
   void get_contacts() {
      // Prepare a request
      out_message_.resize(0);
      cbor::dynamic_buffer buf{out_message_};

      request_t r{get_contacts::request{message_id_++}};
      if (auto res = cbor::encode(buf, r)) {
         throw std::system_error{res};
      }

      std::cout << "-> Get contacts request: " << hex(out_message_) << std::endl;
      server_->handle_message({out_message_}, in_message_);

      // Directly handle the response
      handle_response();
   }

   void add_contact(std::string name, std::string phone, std::optional<std::string> address = std::nullopt) {
      // Prepare a request
      out_message_.resize(0);
      cbor::dynamic_buffer buf{out_message_};

      request_t r{add_contact::request{
         .id = message_id_++,
         .value = {.name = std::move(name), .phone = std::move(phone), .address = std::move(address)},
      }};

      if (auto res = cbor::encode(buf, r)) {
         throw std::system_error{res};
      }

      std::cout << "-> Add contact request: " << hex(out_message_) << std::endl;
      server_->handle_message({out_message_}, in_message_);

      // Directly handle the response
      handle_response();
   }

   void handle_response() {
      // Decode the response
      cbor::read_buffer buf{{in_message_}};

      response_t r{};
      if (auto res = cbor::decode(buf, r)) {
         throw std::system_error{res};
      }

      if (in_message_.size() != buf.read_position()) {
         throw std::runtime_error("Trailing bytes after message");
      }

      // Call an appropriate handler based on the currently active variant alternative.
      // We could also ensure the request and the response ID match here, as well as ensure that we get a correct
      // response type for a request.
      std::visit([&](const auto &r) { this->handle(r); }, r);
   }

   static void handle(const add_contact::response &r) {
      std::cout << std::format("Add a contact result: {}\n", r.result == request_result::success ? "success" : "error");
      if (r.contact_id) {
         std::cout << std::format("Contact ID: {}\n", *r.contact_id);
      } else {
         std::cout << "Contact ID: [empty]\n";
      }
   }

   static void handle(const get_contacts::response &r) {
      std::cout << std::format("Get contacts result: {}\n", r.result == request_result::success ? "success" : "error");
      if (!r.contacts.has_value()) {
         return;
      }

      const auto &contacts = (*r.contacts).contacts;
      if (contacts.empty()) {
         std::cout << "Phone book: [empty]\n";
         return;
      }

      for (const auto &c : contacts) {
         std::cout << "--------------------\n";
         std::cout << std::format("Name:    {}\n", c.name);
         std::cout << std::format("Phone:   {}\n", c.phone);
         if (c.address) {
            std::cout << std::format("Address: {}\n", *c.address);
         } else {
            std::cout << "Address: [not set]\n";
         }
      }
   }

private:
   std::int64_t message_id_{0};
   server *server_{};
   std::vector<std::byte> out_message_{};
   std::vector<std::byte> in_message_{};
};

int main(int, char **) {
   std::cout << "CBOR-based protocol example.\n";
   std::cout << "   Use https://cbor.me/ to check the serialization.\n\n";

   server s;
   client c{s};

   try {
      c.run();
   } catch (const std::runtime_error &e) {
      std::cerr << std::format("Error: {}\n", e.what());
   }

   return 0;
}
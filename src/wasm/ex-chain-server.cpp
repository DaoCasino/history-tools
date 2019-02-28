// copyright defined in LICENSE.txt

#include "ex-chain.hpp"
#include "test-common.hpp"
#include <eosio/database.hpp>

// todo: move
extern "C" void eosio_assert(uint32_t test, const char* msg) {
    if (!test)
        eosio_assert_message(test, msg, strlen(msg));
}

void process(block_info_request& req, const eosio::context_data& context) {
    eosio::print("    block_info_request\n");
    auto s = exec_query(eosio::query_block_info_range_index{
        .first       = get_block_num(req.first, context),
        .last        = get_block_num(req.last, context),
        .max_results = req.max_results,
    });

    block_info_response response;
    eosio::for_each_query_result<eosio::block_info>(s, [&](eosio::block_info& b) {
        response.more = eosio::make_absolute_block(b.block_num + 1);
        response.blocks.push_back(b);
        return true;
    });
    set_output_data(pack(chain_response{std::move(response)}));
    eosio::print("\n");
}

void process(tapos_request& req, const eosio::context_data& context) {
    eosio::print("    tapos_request\n");
    auto s = exec_query(eosio::query_block_info_range_index{
        .first       = get_block_num(req.ref_block, context),
        .last        = get_block_num(req.ref_block, context),
        .max_results = 1,
    });

    tapos_response response;
    eosio::for_each_query_result<eosio::block_info>(s, [&](eosio::block_info& b) {
        uint32_t x                = b.block_id.value.data()[0] >> 32;
        response.ref_block_num    = b.block_num;
        response.ref_block_prefix = (x << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | (x >> 24);
        response.expiration       = b.timestamp;
        response.expiration.slot += req.expire_seconds * 2;
        return true;
    });

    set_output_data(pack(chain_response{std::move(response)}));
    eosio::print("\n");
}

void process(account_request& req, const eosio::context_data& context) {
    eosio::print("    account\n");
    auto s = exec_query(eosio::query_account_range_name{
        .max_block   = get_block_num(req.max_block, context),
        .first       = req.first,
        .last        = req.last,
        .max_results = req.max_results,
    });

    account_response response;
    eosio::for_each_query_result<eosio::account>(s, [&](eosio::account& a) {
        response.more = eosio::name{a.name.value + 1};
        if (a.present) {
            if (!req.include_abi)
                a.abi = {nullptr, 0};
            if (!req.include_code)
                a.code = {nullptr, 0};
            response.accounts.push_back(a);
        }
        return true;
    });
    set_output_data(pack(chain_response{std::move(response)}));
    eosio::print("\n");
}

void process(abis_request& req, const eosio::context_data& context) {
    eosio::print("    abis\n");
    abis_response response;
    for (auto name : req.names) {
        auto s     = exec_query(eosio::query_account_range_name{
            .max_block   = get_block_num(req.max_block, context),
            .first       = name,
            .last        = name,
            .max_results = 1,
        });
        bool found = false;
        eosio::for_each_query_result<eosio::account>(s, [&](eosio::account& a) {
            if (a.present) {
                found = true;
                response.abis.push_back(name_abi{a.name, true, a.abi});
            }
            return true;
        });
        if (!found)
            response.abis.push_back(name_abi{name, false, {nullptr, 0}});
    }
    set_output_data(pack(chain_response{std::move(response)}));
    eosio::print("\n");
}

extern "C" void startup() {
    auto request = eosio::unpack<chain_request>(get_input_data());
    eosio::print("request: ", chain_request::keys[request.value.index()], "\n");
    std::visit([](auto& x) { process(x, eosio::get_context_data()); }, request.value);
}

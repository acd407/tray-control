//
// Created by andrew on 11/17/22.
//
#pragma once

#include <variant>
#include <expected>
#include <ranges>
#include <functional>

template <typename T, typename E> constexpr std::variant<T, E> toVariant(std::expected<T, E> &&expected) {
    if (expected)
        return std::variant<T, E>{std::in_place_index<0>, std::move(expected.value())};
    else
        return std::variant<T, E>{std::in_place_index<1>, std::move(expected.error())};
}

// Helper function to apply a function to the value of an expected if it has a value
template <typename T, typename E, typename F>
constexpr auto mapExpected(std::expected<T, E> &&exp, F &&f) -> std::expected<std::invoke_result_t<F, T>, E> {
    if (exp)
        return std::invoke(std::forward<F>(f), std::move(exp.value()));
    else
        return std::unexpected{exp.error()};
}

// Helper function to apply a function to the value of an expected if it has a value, and ignore the result
template <typename T, typename E, typename F> constexpr void ifExpected(std::expected<T, E> &&exp, F &&f) {
    if (exp) {
        std::invoke(std::forward<F>(f), std::move(exp.value()));
    }
}
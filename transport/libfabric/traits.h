//
// Created by Valentino on 10/08/16.
//

#ifndef TRANSPORT_LIBFABRIC_FABRIC_TRAITS_H
#define TRANSPORT_LIBFABRIC_FABRIC_TRAITS_H

#include <type_traits>
#include <rdma/fabric.h>

namespace lseb {

namespace detail {
template<typename T>
struct _is_fabric_id :
    public std::false_type {
};

template<>
struct _is_fabric_id<fid_mr> :
    public std::true_type {
};

template<>
struct _is_fabric_id<fid_fabric> :
    public std::true_type {
};

template<>
struct _is_fabric_id<fid_domain> :
    public std::true_type {
};

template<>
struct _is_fabric_id<fid_pep> :
    public std::true_type {
};

template<>
struct _is_fabric_id<fid_eq> :
    public std::true_type {
};



}

template<typename T>
struct is_fabric_id
    : public detail::_is_fabric_id<typename std::remove_cv<T>::type> {
};
}

#endif //TRANSPORT_LIBFABRIC_FABRIC_TRAITS_H

namespace PrivateAccess {
template <auto memberPtr>
struct CallPrivateFunctions {
  static constexpr auto kMemPtr = memberPtr;
  struct Delegate;
};

template <auto memberPtr>
struct AccessPrivateVars {
  static constexpr auto kMemPtr = memberPtr;
  struct Delegate;
};
}  // namespace PrivateAccess

#define DEFINE_CALLER(qualified_class_name, class_private_func)              \
  namespace PrivateAccess {                                                  \
  template <>                                                                \
  struct CallPrivateFunctions<                                               \
      &qualified_class_name::class_private_func>::Delegate {                 \
    template <class... Args>                                                 \
    friend auto call_##class_private_func(qualified_class_name& obj,         \
                                          Args&&... args) {                  \
      return (obj.*kMemPtr)(args...);                                        \
    }                                                                        \
  };                                                                         \
  template <class... Args>                                                   \
  auto call_##class_private_func(qualified_class_name& obj, Args&&... args); \
  }

#define DEFINE_ACCESSOR(qualified_class_name, class_data_member)                   \
  namespace PrivateAccess {                                                        \
  template <>                                                                      \
  struct AccessPrivateVars<&qualified_class_name::class_data_member>::Delegate {   \
    friend auto &get_##class_data_member(qualified_class_name &obj) {              \
      return obj.*kMemPtr;                                                         \
    }                                                                              \
  };                                                                               \
  auto &get_##class_data_member(qualified_class_name &obj);                        \
  }
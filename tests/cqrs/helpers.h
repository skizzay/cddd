// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#pragma once
#include "cqrs/artifact_store.h"
#include "cqrs/fakes/fake_event.h"
#include <kerchow/kerchow.h>
#include <fakeit.hpp>
#include <range/v3/view.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace cddd {
namespace cqrs {
namespace {

using namespace fakeit;

using domain_event_pointer = std::shared_ptr<domain_event>;

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};
auto nothing = [](auto &...) {};
auto dont_delete = [](auto *) {};


class stream_stub_implementation final {
public:
   explicit inline stream_stub_implementation(const commit &c, const std::vector<domain_event_pointer> &load_playback) :
      load_playback{load_playback},
      persistance_result(c)
   {
   }

   inline auto load(std::size_t, std::size_t) const noexcept {
      return ranges::view::all(load_playback);
   }

   template<class T>
   inline void save(T) noexcept {
   }

   inline commit persist_changes() {
      return persistance_result;
   }

private:
   std::vector<domain_event_pointer> load_playback;
   commit persistance_result;
};


class stream_abstract_implementation {
   using const_iterator = std::vector<domain_event_pointer>::const_iterator;
   using save_range_type = ranges::iterator_range<const_iterator, const_iterator>;

public:
   virtual ~stream_abstract_implementation() noexcept = default;

   virtual std::vector<domain_event_pointer> load(size_t min_revision, size_t max_revision) const = 0;
   virtual void save(const save_range_type &container) = 0;
};


class source_abstract_implementation {
public:
   virtual ~source_abstract_implementation() noexcept = default;

   virtual bool has_stream_for(const boost::uuids::uuid &) const = 0;
   virtual std::shared_ptr<stream<stream_stub_implementation>> get_stream_for(const boost::uuids::uuid &) const = 0;
   virtual std::shared_ptr<stream<stream_stub_implementation>> create_stream_for(const boost::uuids::uuid &) const = 0;
};


class test_artifact final : public artifact {
public:
   using artifact::basic_artifact;
   using basic_artifact::set_version;

   explicit inline test_artifact() :
      artifact{gen_id()}
   {
   }

   template<class Fun>
   inline void handle(Fun &&f) {
      add_handler(std::forward<Fun>(f));
   }
};


class test_artifact_factory {
public:
   struct interface {
      virtual ~interface() noexcept = default;
      virtual std::shared_ptr<test_artifact> create_test_artifact(const boost::uuids::uuid &id) = 0;
   };

   // For testing, we're allowing implicit construction to make life easier.
   inline test_artifact_factory(interface &pimpl_) :
      pimpl{pimpl_}
   {
   }

   inline auto operator()(const boost::uuids::uuid &id) {
      return pimpl.create_test_artifact(id);
   }

private:
   interface &pimpl;
};


class mock_factory {
   template<class Implementation>
   inline auto create_implementation(Implementation &ref) {
      return std::unique_ptr<Implementation, decltype(dont_delete)>{&ref, dont_delete};
   }

public:
   using abstract_stream = stream<stream_abstract_implementation, decltype(dont_delete)>;
   using stub_stream = stream<stream_stub_implementation>;
   using abstract_source = domain_event_stream_store<source_abstract_implementation, decltype(dont_delete)>;
   using artifact_store_type = artifact_store<test_artifact, abstract_source, test_artifact_factory>;

   inline std::vector<domain_event_pointer> create_event_container() {
      std::vector<domain_event_pointer> result{kerchow::picker.pick<size_t>(1, 30)};
      std::generate_n(begin(result), result.size(), []() { return std::make_shared<fake_event>(); });
      return result;
   }

   inline auto create_stub_stream() {
      return std::make_shared<stub_stream>(std::make_unique<stream_stub_implementation>(persistance_result, load_playback));
   }

   inline auto create_abstract_stream() {
      return std::make_shared<abstract_stream>(create_implementation(stream_spy.get()));
   }

   inline auto create_abstract_source() {
      return std::make_unique<abstract_source>(create_implementation(source_spy.get()));
   }

   inline auto create_simple_artifact_store() {
      source = create_abstract_source();

      return std::make_shared<artifact_store_type>(*source, test_artifact_factory{artifact_factory_spy.get()});
   }

   inline auto create_artifact() {
      return test_artifact{};
   }

   inline commit create_commit() {
      return commit{gen_id(),
                    gen_id(),
                    kerchow::picker.pick<std::size_t>(1),
                    kerchow::picker.pick<std::size_t>(1),
                    boost::posix_time::microsec_clock::universal_time()};
   }

   commit persistance_result = commit::noncommit();
   std::vector<domain_event_pointer> load_playback;
   Mock<stream_abstract_implementation> stream_spy;
   Mock<source_abstract_implementation> source_spy;
   Mock<test_artifact_factory::interface> artifact_factory_spy;
   std::unique_ptr<abstract_source> source;
};

}
}
}

#include "score_multiplier_asset.h"
#include "Engine/Resource/ebx_type_import.h"
#include "Engine/Resource/ebx_carry.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace dingosdk::score_line {
namespace {
namespace e = frostbite::ebx;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
e::Value& field(e::Object& object, std::string_view name) {
    for (auto& f : object.fields) if (f.name == name) return f.value;
    throw std::runtime_error("Native widget field missing: " + std::string(name));
}
const e::Value& field(const e::Object& object, std::string_view name) {
    const auto* f = object.find(name); require(f != nullptr, "Native widget field missing"); return f->value;
}
e::Object& object(e::Document& d, std::size_t i) { return *d.instances.at(i).object; }
const e::Object& object(const e::Document& d, std::size_t i) { return *d.instances.at(i).object; }
e::Object& nested(e::Object& o, std::string_view name) { return *std::get<std::shared_ptr<e::Object>>(field(o,name).data); }
const e::Object& nested(const e::Object& o, std::string_view name) { return *std::get<std::shared_ptr<e::Object>>(field(o,name).data); }
e::Value::Array& array(e::Object& o, std::string_view name) { return std::get<e::Value::Array>(field(o,name).data); }
const e::Value::Array& array(const e::Object& o, std::string_view name) { return std::get<e::Value::Array>(field(o,name).data); }
std::uint64_t integer(const e::Value& v) {
    if (const auto* u=std::get_if<std::uint64_t>(&v.data)) return *u;
    return static_cast<std::uint64_t>(std::get<std::int64_t>(v.data));
}
std::string text(const e::Object& o, std::string_view name) { return std::get<std::string>(field(o,name).data); }
std::uint32_t hash(std::string_view s) { std::uint32_t h=5381; for(unsigned char c:s) h=h*33^c; return h; }
e::Value reference(std::size_t i) { return {e::PointerReference{e::PointerKind::internal,static_cast<std::int32_t>(i)}, {}}; }
std::size_t referred(const e::Value& value) {
    const auto r=std::get<e::PointerReference>(value.data);
    require(r.kind==e::PointerKind::internal && r.index>=0,"Expected an internal widget reference");
    return static_cast<std::size_t>(r.index);
}
template<class Predicate> std::size_t find(const e::Document& d, std::string_view type, Predicate predicate) {
    std::size_t found=d.instances.size();
    for(std::size_t i=0;i<d.instances.size();++i) {
        const auto& instance=d.instances[i];
        if(d.types.at(instance.descriptor).name!=type || !instance.object || !predicate(*instance.object)) continue;
        require(found==d.instances.size(),"Ambiguous native widget donor");found=i;
    }
    require(found<d.instances.size(),"Native widget donor not found");return found;
}
std::size_t named(const e::Document& d,std::string_view type,std::string_view name) {
    return find(d,type,[&](const e::Object& o){return text(o,"DebugInstanceName")==name;});
}
void identity(e::InstanceRecord& instance,unsigned id) {
    // Private authored identities; the low object-id bits follow the new guid.
    const std::array<unsigned char,16> bytes{0x91,0x4c,0x53,0x01,0x82,0x31,0x42,0x76,0x9a,0x21,0x62,0x19,0x52,0x53,0x4d,0};
    for(std::size_t i=0;i<bytes.size();++i)instance.instanceGuid.bytes[i]=static_cast<std::byte>(bytes[i]);
    instance.instanceGuid.bytes[0]=static_cast<std::byte>(0x90+id);
    instance.instanceGuid.bytes[15]=static_cast<std::byte>(id);
    instance.exported=true;
    instance.dataOffset=UINT32_MAX-id;
    auto& flags=field(*instance.object,"Flags");
    flags.data=(integer(flags)&~std::uint64_t{0x01ffffff})|std::uint64_t{0x01534c90+id};
}
std::size_t clone(e::Document& d,std::size_t source,unsigned id) {
    auto instance=d.instances.at(source);instance.object=e::detail::clone_object(*instance.object);
    identity(instance,id);d.instances.push_back(std::move(instance));return d.instances.size()-1;
}
std::size_t model_index(const e::Document& schema,std::string_view name) {
    const auto& collections=array(object(schema,0),"FieldCollections");require(collections.size()==1,"Score schema collections differ");
    const auto& collection=*std::get<std::shared_ptr<e::Object>>(collections.front().data);
    const auto& fields=array(object(schema,referred(field(collection,"Collection"))),"Fields");
    for(std::size_t i=0;i<fields.size();++i)if(text(object(schema,referred(fields[i])),"Name")==name)return i;
    throw std::runtime_error("Score model field missing");
}
}
e::Document build_multiplier_label(const e::Document& multiplier,const e::Document& score_widget,
                                    const e::Document& score_model,const e::Document& label_widget) {
    require(text(object(multiplier,0),"Name")=="UI/Features/HUD/Scoring/ScoringMultiplier_Widget", "Multiplier blueprint differs");
    require(text(object(score_widget,0),"Name")=="UI/Features/HUD/Scoring/ScoringHUD_Default_Widget", "Score blueprint differs");
    const auto count_index=model_index(score_model,"Multiplier"), label_index=model_index(score_model,"LineMultiplier");
    const auto context=integer(field(object(score_model,0),"TypeNameHash"));
    auto result=e::with_imported_type(multiplier,score_widget,"RimeLabelElementData");
    const auto texture=named(result,"RimeTextureElementData","Multiplier_Texture");
    const auto parent=find(result,"RimeContainerElementData",[&](const e::Object& o){
        const auto& values=array(o,"Elements");
        return std::ranges::any_of(values,[&](const e::Value& v){return referred(v)==texture;});
    });
    const auto root_listener=find(result,"DataModelListenerEntityData",[&](const e::Object& o){
        const auto& path=nested(o,"Path");return array(path,"DataPath").empty() && integer(field(path,"ContextId"))==context;
    });
    const auto label_listener=find(result,"DataModelListenerEntityData",[&](const e::Object& o){
        const auto& path=nested(o,"Path");const auto& values=array(path,"DataPath");
        return values.size()==1 && integer(values.front())==label_index && integer(field(path,"ContextId"))==context;
    });
    const auto widget=find(result,"RimeWidgetEntityData",[](const e::Object&){return true;});
    const auto& connections=array(object(result,0),"PropertyConnections");
    require(!connections.empty(),"Native property connection schema missing");
    const auto connection_template=e::detail::clone_value(connections.front());
    std::uint32_t count_port{};
    for(const auto& v:array(object(result,root_listener),"ExpandedFields")) {
        const auto& o=*std::get<std::shared_ptr<e::Object>>(v.data);
        if(integer(field(o,"FieldIndex"))==count_index) count_port=static_cast<std::uint32_t>(integer(field(o,"PropertyHash")));
    }
    require(count_port!=0,"Native multiplier output missing");
    const auto label_source=named(score_widget,"RimeLabelElementData","SequenceScore_Label");
    e::detail::Carrier labels(score_widget,result,true);
    const auto label=labels.instance(label_source);identity(result.instances[label],1);
    auto& display=object(result,label);
    // Use the number texture's exact layout and the native score label's font.
    for(const auto key:{"HorizontalLayout","VerticalLayout","Width","Height","Transform","TransformPivot"})
        field(display,key)=e::detail::clone_value(field(object(result,texture),key));
    field(display,"DebugInstanceName").data=std::string("ReSkate_MultiplierCount");
    field(display,"InstanceNameHash").data=std::uint64_t{hash("ReSkate_MultiplierCount")};
    field(display,"RawText").data=std::string{};
    field(display,"Visible").data=false;
    field(display,"FitWidthToContent").data=false;field(display,"FitHeightToContent").data=false;
    field(display,"HorizontalAlignment").data=std::int64_t{1};
    field(display,"PointSize").data=72.0;
    auto& siblings=array(object(result,parent),"Elements");
    const auto position=std::ranges::find_if(siblings,[&](const e::Value& v){return referred(v)==texture;});
    siblings.insert(position+1,reference(label));
    const auto listener=clone(result,label_listener,2);
    const auto donor_listener=find(label_widget,"DataModelListenerEntityData",[](const e::Object& o){return !array(o,"ExpandedFields").empty();});
    e::detail::Carrier fields(label_widget,result,true);
    e::Value::Array text_fields;
    for(const auto& v:array(object(label_widget,donor_listener),"ExpandedFields")) {
        const auto& o=*std::get<std::shared_ptr<e::Object>>(v.data);
        if(integer(field(o,"PropertyHash"))==hash("Text"))text_fields.push_back(fields.value(v));
    }
    require(text_fields.size()==1,"Native text output missing");
    auto& expanded=field(object(result,listener),"ExpandedFields");
    expanded.data=std::move(text_fields);expanded.arrayHash=field(object(result,root_listener),"ExpandedFields").arrayHash;
    field(object(result,listener),"Expand").data=true;
    field(object(result,listener),"DataType").data=e::TypeReference{};
    std::size_t compare_source=result.instances.size();
    for(std::size_t i=0;i<result.instances.size();++i)
        if(result.types.at(result.instances[i].descriptor).name=="CompareFloatEntityData"){compare_source=i;break;}
    require(compare_source<result.instances.size(),"Native float comparison missing");
    const auto compare=clone(result,compare_source,3);
    field(object(result,compare),"A").data=0.0;
    field(object(result,compare),"B").data=4.0;
    field(object(result,compare),"TriggerOnPropertyChange").data=true;
    field(object(result,compare),"TriggerOnStart").data=true;field(object(result,compare),"AlwaysSend").data=true;
    // All existing NOT nodes have the same schema and no instance references.
    std::size_t not_source=result.instances.size();
    for(std::size_t i=0;i<result.instances.size();++i)
        if(result.types.at(result.instances[i].descriptor).name=="NotOperationEntityData"){not_source=i;break;}
    require(not_source<result.instances.size(),"Native NOT operation missing");
    const auto inverse=clone(result,not_source,4);
    const auto connect=[&](std::size_t from,std::uint32_t output,std::size_t to,std::uint32_t input) {
        auto v=e::detail::clone_value(connection_template);auto& c=*std::get<std::shared_ptr<e::Object>>(v.data);
        field(c,"Source")=reference(from);field(c,"Target")=reference(to);
        field(c,"SourceFieldId").data=static_cast<std::int64_t>(static_cast<std::int32_t>(output));
        field(c,"TargetFieldId").data=static_cast<std::int64_t>(static_cast<std::int32_t>(input));
        field(c,"Flags").data=std::uint64_t{2};array(object(result,0),"PropertyConnections").push_back(std::move(v));
    };
    connect(listener,hash("Text"),label,hash("RawText"));
    connect(root_listener,count_port,compare,hash("A"));
    connect(compare,hash("A>B"),label,hash("Visible"));
    connect(compare,hash("A>B"),inverse,hash("In"));
    connect(inverse,hash("Out"),texture,hash("Visible"));
    auto& components=array(object(result,widget),"Components");
    for(const auto i:{listener,compare,inverse})components.push_back(reference(i));
    e::detail::sort_instances(result);
    return result;
}
}

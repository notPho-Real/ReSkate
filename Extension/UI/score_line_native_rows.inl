// Reuses the score-line row factory and the original game's trick stack.
struct NativeRow {
    ExtraTrick trick;
    Row row;
    std::vector<std::pair<std::uint64_t, std::string>> older;
};
struct NativeRows {
    std::string owner;
    std::uint64_t requested_root{};
    std::vector<ExtraTrick> requested;
    double at{};
    Address manager{};
    Value root;
    std::vector<NativeRow> rows;
};
NativeRows& native_rows() { static auto* value = new NativeRows; return *value; }
bool has_native_rows() { const auto& n=native_rows(); return !n.rows.empty() || !n.requested.empty(); }
void forget_native_rows() { auto& n=native_rows(); n.manager=0;n.root={};n.rows.clear(); }
struct NativeStackRow { std::vector<std::byte> bytes; Widget widget; std::string name; };
std::vector<NativeStackRow> native_stack_rows(Hud& h, const Context& c, Value root) {
    unsigned count{},stride{};
    const auto bytes=c.array(c.path(root,{score::trick_list,corner::stack_items}),64,count,stride);
    require(stride==menu_data::stack_item.size && bytes.size()==std::size_t{count}*stride,
        "Native trick stack differs.");
    std::vector<NativeStackRow> result;
    const auto& owned=native_rows().rows;
    for(unsigned i=0;i<count;++i) {
        const auto begin=bytes.begin()+static_cast<std::ptrdiff_t>(std::size_t{i}*stride);
        NativeStackRow row;row.bytes.assign(begin,begin+stride);
        require(h.content_at+sizeof(Widget)<=row.bytes.size(),"Native trick item differs.");
        std::memcpy(&row.widget,row.bytes.data()+h.content_at,sizeof row.widget);
        if(std::any_of(owned.begin(),owned.end(),[&](const NativeRow& ours){
            return row.widget.data.handle==ours.row.model.handle && row.widget.blueprint==h.row_blueprint;
        }))continue;
        // Names only anchor presentation ordering; no score is inferred here.
        if(row.widget.data.handle && row.widget.blueprint==h.row_blueprint) {
            const Value model{row.widget.data.handle,c.type_of(row.widget.data.handle)};
            if(model.type)row.name=c.text(label_text(c,model,score::row_name),128);
        }
        result.push_back(std::move(row));
    }
    return result;
}
void publish_native_rows(const Context& c, Value root, const std::vector<std::byte>& bytes, unsigned count) {
    const auto target=c.path(root,{score::trick_list,corner::stack_items});
    unsigned old_count{},stride{};
    const auto old=c.array(target,64,old_count,stride);
    if(old_count!=count || old!=bytes)c.array(target,bytes,count);
}
void clear_native_rows(Hud& h,const Context& c) {
    auto& n=native_rows();
    if(n.manager!=c.manager) { forget_native_rows();return; }
    if(n.rows.empty())return;
    if(alive(c,n.root)) {
        const auto native=native_stack_rows(h,c,n.root);
        std::vector<std::byte> bytes;
        for(const auto& row:native)bytes.insert(bytes.end(),row.bytes.begin(),row.bytes.end());
        publish_native_rows(c,n.root,bytes,static_cast<unsigned>(native.size()));
    }
    for(auto& row:n.rows) {destroy_quietly(c,row.row.item);destroy_quietly(c,row.row.model);}
    forget_native_rows();
    tricks::feed::own({});
}
void update_native_rows(Hud& h,const Context& c,bool allowed,double now) {
    auto& n=native_rows();
    if(n.manager && n.manager!=c.manager)forget_native_rows();
    if(!allowed || n.requested.empty() || now-n.at>lease_ms ||
        n.requested_root!=h.game_root.handle || !alive(c,h.game_root) || !h.row_blueprint) {
        clear_native_rows(h,c);return;
    }
    if(n.root.handle && n.root.handle!=h.game_root.handle)clear_native_rows(h,c);
    n.manager=c.manager;n.root=h.game_root;
    const auto native=native_stack_rows(h,c,n.root);
    const auto type=row_type_now(h,c,now);
    if(!type)return;
    // Keep removed models alive until publication has removed their references.
    std::vector<NativeRow> next;
    std::vector<NativeRow> made;
    try {
        for(const auto& requested:n.requested) {
            const auto existing=std::find_if(n.rows.begin(),n.rows.end(),[&](const NativeRow& row){
                return row.trick.occurrence==requested.occurrence && row.trick.scope==requested.scope && row.trick.name==requested.name;
            });
            if(existing!=n.rows.end()) {next.push_back(*existing);continue;}
            NativeRow row;row.trick=requested;row.row=make_row(h,c,type,requested.name);
            made.push_back(row);
            for(const auto& old:native)row.older.emplace_back(old.widget.data.handle,old.name);
            next.push_back(std::move(row));
        }
        std::vector<std::uint64_t> entries;
        for(const auto& row:n.rows)entries.push_back(row.row.model.handle>>16);
        for(const auto& row:made)entries.push_back(row.row.model.handle>>16);
        require(entries.size()<=tricks::feed::most_owned,"Too many owned native trick rows.");
        tricks::feed::own(entries); // before native widgets can publish their labels
        std::vector<std::byte> bytes;
        std::vector<std::size_t> positions;
        for(const auto& row:next) {
            std::size_t position=native.size();
            for(std::size_t i=0;i<native.size();++i)if(std::find(row.older.begin(),row.older.end(),
                std::make_pair(native[i].widget.data.handle,native[i].name))!=row.older.end()){position=i;break;}
            positions.push_back(position);
        }
        for(std::size_t i=0;i<=native.size();++i) {
            // Caller order is oldest first; native stack order is newest first.
            for(std::size_t j=next.size();j>0;--j)if(positions[j-1]==i) {
                const auto item=bytes_of(c,next[j-1].row.item);bytes.insert(bytes.end(),item.begin(),item.end());
            }
            if(i<native.size())bytes.insert(bytes.end(),native[i].bytes.begin(),native[i].bytes.end());
        }
        require(native.size()+next.size()<=64,"Native trick stack is full.");
        publish_native_rows(c,n.root,bytes,static_cast<unsigned>(native.size()+next.size()));
    } catch (...) {
        // A failed publication may already have retained a row. Keep it tracked
        // so the next cleanup removes it before destroying its model.
        for(auto& row:made)n.rows.push_back(std::move(row));
        throw;
    }
    for(auto& old:n.rows)if(std::none_of(next.begin(),next.end(),[&](const NativeRow& row){return row.row.model.handle==old.row.model.handle;})) {
        destroy_quietly(c,old.row.item);destroy_quietly(c,old.row.model);
    }
    n.rows=std::move(next);
    std::vector<std::uint64_t> entries;
    for(const auto& row:n.rows)entries.push_back(row.row.model.handle>>16);
    tricks::feed::own(entries);
}

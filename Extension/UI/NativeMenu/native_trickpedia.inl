// The live 20260929 screen registers PageViewModel directly. Its authored
// Back command identifies Skatepedia; its resource wrapper is not a live root.
// Retain every original field without changing localized strings or trick assets.
struct TrickpediaPageLease {
    Address manager{};
    Value root{}, target{}, saved{}, replacement{};
    std::uint64_t seen{}, next_scan{};
    bool back_held{};
    Address back_input{};
    std::string back_navigation;
    bool back_active{};
};
TrickpediaPageLease trickpedia_page;
std::atomic<std::uint64_t> trickpedia_seen{};
void hold_trickpedia_back(const Context& c,bool held) {
    auto& lease=trickpedia_page;
    if(held==lease.back_held)return;
    const auto action=c.path(lease.target,{0x10810f0b,button_field,0xa704272a,0xc52416ef,0xa704272a});
    const auto back=c.field(action,0xa2b71c93);
    if(held) {
        // Like the native Hub, retain only the fields we temporarily change.
        // The rest of this action continues to belong to native navigation.
        lease.back_input=read<Address>(c.address(c.field(back,0x3d00de27)));
        lease.back_navigation=c.text(c.field(back,0xb1e14cd6));
        lease.back_active=read<bool>(c.address(c.field(action,0x4b3146e9)));
        lease.back_held=true;
        c.set(c.field(back,0x3d00de27),Address{});
        c.text(c.field(back,0xb1e14cd6),"");
        c.set(c.field(action,0x4b3146e9),true);
    } else {
        c.set(c.field(back,0x3d00de27),lease.back_input);
        c.text(c.field(back,0xb1e14cd6),lease.back_navigation);
        c.set(c.field(action,0x4b3146e9),lease.back_active);
        lease.back_held=false;
    }
}
bool restore_trickpedia(const Context& c) {
    auto& lease=trickpedia_page;
    trickpedia_seen.store(0,std::memory_order_release);
    if(!lease.manager)return true;
    if(lease.manager!=c.manager) {lease={};return true;}
    // Handles are revalidated before every access; a released screen needs no write.
    if(c.type_of(lease.root.handle)==lease.root.type && c.type_of(lease.target.handle)==lease.target.type)
    {
        // The screen record belongs to the native navigation owner and can
        // change while our body is shown. Restore only fields we replaced.
        hold_trickpedia_back(c,false);
        for(const auto field:{0x716496c8U,0x3781b603U})
            c.copy(c.field(lease.target,field),c.address(c.field(lease.saved,field)));
    }
    c.destroy(lease.replacement);c.destroy(lease.saved);lease={};return true;
}
void update_trickpedia(const Context& c,std::uint64_t now) {
    auto& lease=trickpedia_page;
    if(lease.manager && lease.manager!=c.manager){lease={};trickpedia_seen.store(0,std::memory_order_release);}
    if(!arcade_mode::enabled()) {restore_trickpedia(c);return;}
    if(lease.manager) {
        if(c.type_of(lease.root.handle)!=lease.root.type || c.type_of(lease.target.handle)!=lease.target.type) {
            restore_trickpedia(c);return;
        }
        const auto focus=c.path(lease.target,{0xfafda1a6,0x20a39d92,0x6b01fd01});
        const bool shown=game_in_menu.load(std::memory_order_relaxed) && read<int>(c.address(focus))>=0;
        if(!shown){restore_trickpedia(c);return;}
        hold_trickpedia_back(c,back_wanted.load(std::memory_order_relaxed));
        trickpedia_seen.store(shown?now:0,std::memory_order_release);
        return;
    }
    if(now<lease.next_scan)return;
    lease.next_scan=now+250;
    const auto& source=page_state(menu_view::tools_page);
    if(source.manager!=c.manager || !source.page.handle || c.type_of(source.page.handle)!=source.page.type)return;
    for(const auto& root:c.roots({page.hash})) {
        const auto target=root.model;
        if(target.type!=c.type(page))continue;
        const auto focus=c.path(target,{0xfafda1a6,0x20a39d92,0x6b01fd01});
        if(!game_in_menu.load(std::memory_order_relaxed) || read<int>(c.address(focus))<0)continue;
        const auto back=c.path(target,{0x10810f0b,button_field,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
        if(c.text(c.field(back,0xb1e14cd6))!="SP_Back")continue;
        // Two engine-owned copies retain nested arrays, strings and asset references.
        // Nothing is exposed before both copies and the original Back action exist.
        Value saved{},replacement{};
        try {
            saved=c.create(page,0x545249434b534156ULL);
            c.copy(saved,c.address(target));
            replacement=c.create(page,0x545249434b504544ULL);
            c.copy(replacement,c.address(target));
            c.copy(c.field(replacement,0x716496c8U),c.address(c.field(source.page,0x716496c8U)));
            c.text(c.path(replacement,{0x3781b603,text_field}),"ARCADE TRICKPEDIA");
            lease={c.manager,root.model,target,saved,replacement,now,now+250,false,0,{},false};
            // Replacing the whole Page overwrites its live screen state and
            // leaves the overlay visible after native Back. Keep that owner.
            for(const auto field:{0x716496c8U,0x3781b603U})
                c.copy(c.field(target,field),c.address(c.field(replacement,field)));
            trickpedia_seen.store(now,std::memory_order_release);
            return;
        } catch(...) {
            // If publication began, keep the retained original for the next cleanup attempt.
            if(lease.manager) {restore_trickpedia(c);throw;}
            if(replacement.handle)c.destroy(replacement);
            if(saved.handle)c.destroy(saved);
            throw;
        }
    }
}

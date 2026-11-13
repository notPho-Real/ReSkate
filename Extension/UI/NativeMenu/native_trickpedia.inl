// The live 20260929 screen registers PageViewModel directly. Its authored
// Back command identifies Skatepedia; its resource wrapper is not a live root.
// Retain every original field without changing localized strings or trick assets.
struct TrickpediaPageLease {
    Address manager{};
    Value root{}, target{}, saved{}, replacement{};
    std::uint64_t seen{}, next_scan{};
    bool back_held{};
};
TrickpediaPageLease trickpedia_page;
std::atomic<std::uint64_t> trickpedia_seen{};
bool restore_trickpedia(const Context& c) {
    auto& lease=trickpedia_page;
    trickpedia_seen.store(0,std::memory_order_release);
    if(!lease.manager)return true;
    if(lease.manager!=c.manager) {lease={};return true;}
    // Handles are revalidated before every access; a released screen needs no write.
    if(c.type_of(lease.root.handle)==lease.root.type && c.type_of(lease.target.handle)==lease.target.type)
        c.copy(lease.target,c.address(lease.saved));
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
        const bool held=back_wanted.load(std::memory_order_relaxed);
        if(held!=lease.back_held) {
            if(held) {
                // Use the existing Page Back action's input-swallowing state.
                // Restoring copies the original navigation, never ToggleMenu.
                const auto action=c.path(lease.target,{0x10810f0b,button_field,0xa704272a,0xc52416ef,0xa704272a});
                const auto back=c.field(action,0xa2b71c93);
                c.set(c.field(back,0x3d00de27),Address{});
                c.text(c.field(back,0xb1e14cd6),"");
                c.set(c.field(action,0x4b3146e9),true);
            } else c.copy(c.field(lease.target,276893451U),c.address(c.field(lease.saved,276893451U)));
            lease.back_held=held;
        }
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
            c.copy(replacement,c.address(source.page));
            c.copy(c.field(replacement,276893451U),c.address(c.field(saved,276893451U)));
            c.text(c.path(replacement,{0x3781b603,text_field}),"ARCADE TRICKPEDIA");
            c.set(c.path(replacement,{0xfafda1a6,0x20a39d92,0x6b01fd01}),0);
            lease={c.manager,root.model,target,saved,replacement,now,now+250};
            c.copy(target,c.address(replacement));
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

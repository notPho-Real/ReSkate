namespace skatepedia_fields=addr::skatepedia;
constexpr Schema skatepedia_card{skatepedia_fields::info_card_schema,skatepedia_fields::info_card_size};
struct TrickpediaPageLease {
    Address manager{};
    Value page{}, card{}, saved{}, replacement{};
    std::string title;
    std::vector<std::string> applied;
    std::uint64_t next_scan{};
    ControllerStyle controller_style{};
    arcade_mode::ArcadeTrick selected_grab{};
};
TrickpediaPageLease trickpedia_page;
std::vector<std::string> card_inputs(const Context& c,Value card) {
    const auto inputs=c.field(card,skatepedia_fields::inputs);
    unsigned count{},stride{};
    (void)c.array(inputs,16,count,stride);
    std::vector<std::string> out;
    for(unsigned i=0;i<count;++i)out.push_back(c.text(c.field(c.element(inputs,i),skatepedia_fields::text),2048));
    return out;
}
std::string card_title(const Context& c,Value card) {
    return c.text(c.path(card,{skatepedia_fields::title,skatepedia_fields::text}));
}
bool restore_trickpedia(const Context& c) {
    auto& lease=trickpedia_page;
    if(!lease.manager)return true;
    if(lease.manager!=c.manager){lease={};return true;}
    // Native selection can replace this same card in place. Restore only an
    // input array we still own, never stale inputs over the newly selected trick.
    if(c.type_of(lease.card.handle)==lease.card.type &&
        card_title(c,lease.card)==lease.title && card_inputs(c,lease.card)==lease.applied)
        c.copy(c.field(lease.card,skatepedia_fields::inputs),c.address(c.field(lease.saved,skatepedia_fields::inputs)));
    c.destroy(lease.replacement);c.destroy(lease.saved);lease={};return true;
}
bool skatepedia_visible(const Context& c,Value target) {
    if(!game_in_menu.load(std::memory_order_relaxed) || c.type_of(target.handle)!=target.type)return false;
    const auto focus_value=c.path(target,{skatepedia_fields::focus,skatepedia_fields::focus_state,skatepedia_fields::focus_index});
    if(read<int>(c.address(focus_value))<0)return false;
    const auto back_value=c.path(target,{skatepedia_fields::back,skatepedia_fields::button,skatepedia_fields::action,skatepedia_fields::action_state,
        skatepedia_fields::action,skatepedia_fields::navigation,skatepedia_fields::navigation_name});
    return c.text(back_value)=="SP_Back";
}
void update_trickpedia(const Context& c,std::uint64_t now) {
    auto& lease=trickpedia_page;
    if(lease.manager && lease.manager!=c.manager)lease={};
    if(!arcade_mode::enabled() || !game_in_menu.load(std::memory_order_relaxed)){restore_trickpedia(c);return;}
    ControllerInput controller{};DingoSDKOverlayReadControllerInput(&controller,true);
    const auto selected_grab=arcade_mode::selected_grab();
    if(lease.manager) {
        if(lease.controller_style==controller.style && lease.selected_grab==selected_grab &&
            skatepedia_visible(c,lease.page) && c.type_of(lease.card.handle)==lease.card.type &&
            card_title(c,lease.card)==lease.title && card_inputs(c,lease.card)==lease.applied)return;
        restore_trickpedia(c);
    }
    if(now<lease.next_scan)return;
    lease.next_scan=now+250;
    Value native_page{};
    for(const auto& root:c.roots({page.hash}))if(skatepedia_visible(c,root.model)){native_page=root.model;break;}
    if(!native_page.handle)return;
    for(const auto& root:c.roots({skatepedia_card.hash})) {
        const auto card=root.model;
        if(card.type!=c.type(skatepedia_card))continue;
        const auto title=card_title(c,card);
        const auto binding=arcade_mode::trickpedia::native_card_binding(title);
        const auto* trick=binding.trick;
        if(!trick && !binding.planned)continue;
        unsigned count{},stride{};
        const auto original=c.array(c.field(card,skatepedia_fields::inputs),16,count,stride);
        if(!count || !stride)continue;
        const auto guidance=trick ? arcade_mode::trickpedia::native_card_guidance(*trick,
            [&](std::uint32_t buttons){return controller_combo_label(buttons,controller.style);},selected_grab,binding.nollie) :
            arcade_mode::trickpedia::NativeCardGuidance{{"Arcade controls for this variation are not available yet."},"Planned"};
        std::string text;
        for(const auto& input:guidance.inputs){if(!text.empty())text+="\n";text+=input;}
        text+="\n"+guidance.status;
        Value saved{},replacement{};
        try {
            saved=c.create(skatepedia_card,0x534b415445534156ULL);c.copy(saved,c.address(card));
            replacement=c.create(skatepedia_card,0x534b415445494e50ULL);c.copy(replacement,c.address(card));
            const auto inputs=c.field(replacement,skatepedia_fields::inputs);
            // Reuse the first native label's styling. The typed array writer
            // retains its nested values before resizing; no borrowed text escapes.
            c.array(inputs,std::span<const std::byte>(original.data(),stride),1);
            c.text(c.field(c.element(inputs,0),skatepedia_fields::text),text);
            lease={c.manager,native_page,card,saved,replacement,title,{text},now+250,controller.style,selected_grab};
            c.copy(c.field(card,skatepedia_fields::inputs),c.address(inputs));
            return;
        } catch(...) {
            if(lease.manager){restore_trickpedia(c);throw;}
            if(replacement.handle)c.destroy(replacement);
            if(saved.handle)c.destroy(saved);
            throw;
        }
    }
}

// Included after the existing Hub page layout/input helpers. No second menu input owner.
void trickpedia_page(Ui& ui,ImVec2 a,ImVec2 z) {
    namespace guide=arcade_mode::trickpedia;
    using namespace arcade_mode;
    auto& h=ui.h;auto& query=h.trick_query;const float u=ui.u;
    if(!arcade_mode::enabled()) {
        label(ui.draw,ui.body,40*u,a,z.x,theme::paper,"Arcade Mode is off. Open Skatepedia for the original controls.");
        return;
    }
    const auto old_query=query.search;const auto old_category=query.category,old_status=query.status;
    const float split=a.x+(z.x-a.x)*.42f;
    // A controller/Enter activation opens ordinary focusable buttons, so search
    // never requires an external keyboard or a separate input owner.
    if(h.fire==hash("trick-search")) {
        h.fire=0;set_typing(h,0);h.trick_keyboard=true;
        h.focus_next=hash("trick-key",1);
        dingosdk::multiplayer::hold_native_menu_back(true);
    }
    const bool search_typing=h.typing==hash("trick-search");
    field(ui,hash("trick-search"),a,ImVec2(split-24*u,a.y+82*u),query.search,"SEARCH TRICKS");
    if(search_typing && h.typing!=hash("trick-search"))h.trick_back_until=GetTickCount64()+300;
    if(h.typing==hash("trick-search"))dingosdk::multiplayer::hold_native_menu_back(true);
    const auto category=query.category<0?"All categories":guide::category_names[static_cast<unsigned>(query.category)];
    const auto status=query.status<0?"All statuses":guide::status_names[static_cast<unsigned>(query.status)];
    if(button(ui,hash("trick-category"),ImVec2(split,a.y),ImVec2(split+(z.x-split)*.6f-12*u,a.y+82*u),category))
        query.category=query.category+1==static_cast<int>(guide::category_names.size())?-1:query.category+1;
    if(button(ui,hash("trick-status"),ImVec2(split+(z.x-split)*.6f,a.y),ImVec2(z.x,a.y+82*u),status))
        query.status=query.status==2?-1:query.status+1;
    if(query.search!=old_query || query.category!=old_category || query.status!=old_status) {
        h.scrolls[hash("trick-list")]=0;h.scrolls[hash("trick-detail")]=0;
    }
    if(h.trick_keyboard) {
        const float width=(z.x-a.x)/8.f,height=std::min(96.f*u,(z.y-a.y-110.f*u)/5.f);
        for(unsigned key=0;key<guide::search_keys.size();++key) {
            const ImVec2 start(a.x+static_cast<float>(key%8)*width,a.y+110*u+static_cast<float>(key/8)*height);
            if(button(ui,hash("trick-key",key+1),start,ImVec2(start.x+width-12*u,start.y+height-12*u),std::string(guide::search_keys[key]))) {
                if(guide::search_key(query,key)) {
                    h.trick_keyboard=false;h.focus_next=hash("trick-search");
                }
                h.scrolls[hash("trick-list")]=0;h.scrolls[hash("trick-detail")]=0;
            }
        }
        return;
    }
    const auto entries=guide::browse(query);
    const auto selected=std::find_if(entries.begin(),entries.end(),[&](const auto* t){return t->id==h.selected_trick;});
    if(selected==entries.end())h.selected_trick=entries.empty()?ArcadeTrick::none:entries.front()->id;
    Scroller list(ui,hash("trick-list"),ImVec2(a.x,a.y+110*u),ImVec2(split-24*u,z.y));
    int previous_category=-1;
    for(const auto* t:entries) {
        const auto group=static_cast<int>(t->category);
        if(group!=previous_category) {
            bool drawn{};const float top=list.next(58*u,10*u,drawn);
            if(drawn)label(ui.draw,ui.body,32*u,ImVec2(a.x+16*u,top+12*u),split-36*u,
                theme::muted,guide::category_names[static_cast<unsigned>(group)]);
            previous_category=group;
        }
        bool drawn{};const float top=list.next(88*u,10*u,drawn);
        const auto id=hash(t->key,47);
        if(button(ui,id,ImVec2(a.x,top),ImVec2(split-36*u,top+88*u),std::string(t->name),
                  h.selected_trick==t->id?Look::chosen:Look::plain,true,
                  guide::evidence_row_label(*t),&list.box)) {
            h.selected_trick=t->id;h.scrolls[hash("trick-detail")]=0;
        }
        list.reveal(id,top,88*u);
    }
    list.end();
    const auto* t=trick_definition(h.selected_trick);if(!t)return;
    Scroller detail(ui,hash("trick-detail"),ImVec2(split,a.y+110*u),z);
    unsigned detail_line=0;
    const auto line=[&](const std::string& text,ImU32 color=theme::paper,float height=84.f) {
      for(const auto& fragment:guide::detail_chunks(text)) {
        const float wrap=std::max(100*u,z.x-split-44*u);
        const float pixels=std::max(height*u,ui.body->CalcTextSizeA(32*u,FLT_MAX,wrap,fragment.c_str()).y+24*u);
        bool drawn{};const float top=detail.next(pixels,8*u,drawn);
        if(drawn)ui.draw->AddText(ui.body,32*u,ImVec2(split+20*u,top+12*u),color,fragment.c_str(),nullptr,wrap);
        // A focusable line lets controller users scroll through every alternative.
        const auto id=hash(fragment,++detail_line);item(ui,id,ImVec2(split,top),ImVec2(z.x-16*u,top+pixels),&detail.box);
        detail.reveal(id,top,pixels);
      }
    };
    line(std::string(t->name));
    line(guide::evidence_label(*t),theme::muted);
    if(!t->binding_count)line("Controls not assigned.");
    for(unsigned i=0;i<t->binding_count;++i) {
        const auto& b=t->bindings[i];const auto d=guide::diagram(*t,b,arcade_mode::selected_grab());
        line((d.proposed?"Proposed: ":"")+guide::diagram_text(d,[&](std::uint32_t buttons){return controller_combo_label(buttons,ui.pad.style);}));
        line(d.condition,theme::muted);
        if(d.window_ms)line("Input window: "+std::to_string(d.window_ms)+" ms",theme::muted);
        if(b.pattern==BindingPattern::selected_grab && b.variant!=arcade_mode::selected_grab()) {
            bool drawn{};const float top=detail.next(84*u,8*u,drawn);
            const auto id=hash("select-grab",static_cast<unsigned>(b.variant));
            if(button(ui,id,ImVec2(split,top),ImVec2(z.x-16*u,top+84*u),"USE THIS R3 + B POSE",Look::tile,true,{},&detail.box))
                arcade_mode::set_selected_grab(b.variant);
            detail.reveal(id,top,84*u);
        }
    }
    if(t->executor==TrickExecutor::custom_flatland) {
        line("While held: "+controller_combo_label(TrickControls::trigger_mask(0),ui.pad.style)+" or "+
            controller_combo_label(TrickControls::trigger_mask(1),ui.pad.style)+" rotates; hold both to stop turning.",theme::muted);
        line("Press "+controller_combo_label(TrickControls::mask(TrickControls::ollie),ui.pad.style)+" to Ollie out.",theme::muted);
    }
    line("Direction: left stick / D-pad. Release between repeated presses.",theme::muted);
    if(t->nollie_flip)line("Native regular and nollie variants; live stance admission applies.",theme::muted);
    line(std::string(t->evidence.remaining),theme::muted);
    {
        bool drawn{};const float top=detail.next(84*u,8*u,drawn);
        const auto id=hash("trick-evidence-details");
        if(button(ui,id,ImVec2(split,top),ImVec2(z.x-16*u,top+84*u),
                  h.trick_evidence_details?"HIDE TRIAL DETAILS":"SHOW TRIAL DETAILS",Look::tile,true,{},&detail.box))
            h.trick_evidence_details=!h.trick_evidence_details;
        detail.reveal(id,top,84*u);
    }
    for(const auto& text:guide::evidence_details(*t,h.trick_evidence_details))line(text,theme::muted);
    detail.end();
}

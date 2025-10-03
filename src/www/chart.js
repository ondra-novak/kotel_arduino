//@html chart.html
//@style chart.css
//@require svg.js
//@require websocketclient.js
//@require protocol.js

/// <reference path="./svg.js"/>
/// <reference path="./websocketclient.js"/>
/// <reference path="./protocol.js"/>

const daymseconds= 24*60*60*1000


function formatDateRoman(date) {
    const day = date.getDate();
    const month = date.getMonth() + 1;
    const romanMonths = [
        "I", "II", "III", "IV", "V", "VI",
        "VII", "VIII", "IX", "X", "XI", "XII"
    ];
    return `${day}.${romanMonths[month - 1]}.`;
}

async function openChart(ws /*:WebSocketExchange*/, fdspeed) {
    const dlg = ModalDialog.load("dailyChart");
    const flds = dlg.get_fields();
    const day_end = Math.floor(new Date().getTime()/daymseconds);
    const day_start = day_end - 360;
    const mdata = [];
    const  loadData =async () => {
        for (let i = day_end-1; i >= day_start; i-=7) {
            let b = i - 7;
            let e = Math.max(day_start, i);
            try {
                const d = await ws.send_request(WsReqCmd.history, `from=${b}&to=${e}`)
                d.split('\n',e-b).map((ln,ofs)=>{
                    let [day,f,fl,fu,e1,e2,c1,c2]
                        = ln.split(',')
                        .map((x,idx)=>idx?parseFloat(x):(new Date(x)));
                    day = formatDateRoman(day);
                    f = Math.round(f / fdspeed * 2)/2;
                    fl = Math.round(fl / fdspeed * 2)/2;
                    mdata[b-day_start+ofs] ={day,f,fl,fu,e1,e2,c1,c2};                        
                });                
                redraw();
            } catch (e) {

            }
        }
    }

    const redraw = ()=>{
        const chart = flds.chart;
        const rulery = flds.rulery;
        chart.clear();
        rulery.clear();
        const md = mdata.filter(x=>x);
        const maxF = Math.max(5,...md.map(d => d.f),...md.map(d=>d.fu/10))*1.1;
        const multf = 480/maxF;

        const to_x = idx=>idx*4;
        const to_y = idx=>multf*(maxF-idx);

        const paths = ["f","fl","fu","e1","e2","c1","c2"].reduce((a,b)=>{
            a[b] = new SvgPathBuilder(b);
            a[b].moveTo(0,to_y(0));
            return a;
        },{});
        const levels = new SvgPathBuilder("l");
        const dates = new SvgPathBuilder("d");
        const btnlbl = new SvgTextGroupBuilder("lbb");
        const lftlbl = new SvgTextGroupBuilder("lbl");
        const fllbl = new SvgTextGroupBuilder("flbl");
        let pd = "";
        md.forEach((d, idx) => {
            const x = to_x(idx);
            paths.f.lineTo(x, to_y(d.f));
            paths.fl.lineTo(x, to_y(d.fl));
            if (d.fu) {
                paths.fu.moveTo(x-1,to_y(0)).lineTo(x-1, to_y(d.fu/10)).horizontalLineTo(to_x(idx+1)+2).verticalLineTo(to_y(0));
                fllbl.add(x,to_y(d.fu/10), `${d.fu}kg`);
            }
            ["e1","e2","c1","c2"].forEach(n=>{
                if (d[n]) {
                    paths[n].moveTo(x,to_y(0)).lineTo(x, to_y(d[n])).horizontalLineTo(to_x(idx+1)).verticalLineTo(to_y(0));
                }
            });
            const m = d.day.split(".")[1];
            if (pd != m) {
                pd = m;
                dates.moveTo(x,0).verticalLineTo(to_y(0));
                btnlbl.add(x,to_y(0), m);
            }
        });
        const maxx = to_x(md.length)
        for (let y = 0; y < maxF; y+= 5) {
            levels.moveTo(maxx,to_y(y)).horizontalLineTo(-maxx);
            lftlbl.add(27, to_y(y), y);
        }
        
        chart.append(dates.build());
        chart.append(levels.build());
        paths.fl.lineTo(maxx,to_y(0)).closePath();
        Object.values(paths).forEach(
            p=>chart.append(p.build()));        
        const chrel = chart.element();
        if (chrel.parentElement) {
            chrel.parentElement.scrollLeft = chrel.parentElement.scrollWidth;
        }
        const maxy = to_y(0)+20;
        chart.append(btnlbl.build());
        chart.append(fllbl.build());
        chart.viewbox(0,0,maxx, maxy);        
        
        
        rulery.append(lftlbl.build());
        rulery.viewbox(0,0,30, maxy);
    }

    loadData();

    
    return dlg.do_modal("bst");   
}

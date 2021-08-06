const fetch = require('node-fetch');
const express = require('express');

const app = express();
const port = process.env.PORT;

const deviceId = process.env.DEVICE_ID;
const accessToken = process.env.ACCESS_TOKEN;

// add CORS headers
app.use(function(req, res, next) {
    res.header("Access-Control-Allow-Credentials", "true");
    res.header("Access-Control-Allow-Origin", req.headers.origin);
    res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
    res.header("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, OPTIONS");
    next();
});

app.get('/stats', async (req, res) => {
    let iores = await fetch("https://api.particle.io/v1/devices/"+deviceId+"/stats", {
        method: 'POST',
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: new URLSearchParams({
            'access_token': accessToken
        })
    });

    iores = await iores.json();

    let stats = iores.return_value;

    let soc = stats >> 16;
    soc /= 10; // soc is in per mill
    //console.log("SoC: " + soc + "%");

    let watts = stats & 0xffff;
    if (watts >= 0x8000) watts -= 0x10000;
    watts /= 10; // is in 10th of W
    //console.log("P (W): " + watts);

    res.json({
        soc,
        watts
    })
})

app.listen(port, () => {
    console.log(`Get stats on http://localhost:${port}/stats`)
})

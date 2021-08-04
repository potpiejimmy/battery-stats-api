const fetch = require('node-fetch');
const express = require('express');

const app = express();
const port = process.env.PORT;

const deviceId = process.env.DEVICE_ID;
const accessToken = process.env.ACCESS_TOKEN;

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
    //console.log("SoC: " + soc + "%");

    let current = stats & 0xffff;
    if (current >= 0x8000) current -= 0x10000;
    current /= 100; // is in cA
    //console.log("I (A): " + current / 100);

    res.json({
        soc,
        current
    })
})

app.listen(port, () => {
    console.log(`Get stats on http://localhost:${port}/stats`)
})

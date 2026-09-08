# WireGuard naast Tailscale op Ubuntu 24.04

Ja, Tailscale en een eigen WireGuard-interface kunnen tegelijk op dezelfde
server draaien. Gebruik voor de eigen tunnel een apart interface, een eigen
UDP-poort en een IP-subnet dat niet overlapt met Tailscale, Docker, het LAN of
andere VPN's.

Deze handleiding begint bewust met de veiligste variant: gebruikers krijgen
alleen toegang tot de Ubuntu-server via WireGuard. Routing naar een LAN of het
internet wordt pas daarna, expliciet, toegevoegd.

> Werk op een remote server eerst via je bestaande SSH/Tailscale-verbinding.
> Verwijder Tailscale en verander de bestaande SSH/firewallregels niet voordat
> minstens één WireGuard-client getest is. Houd tijdens de test een tweede
> beheersessie open.

## Model: één peer per gebruiker of apparaat

WireGuard kent peers en cryptografische sleutels, geen gebruikersaccounts.
Geef daarom ieder apparaat een eigen combinatie van:

- private/public keypair;
- preshared key (PSK) tussen precies die client en de server;
- uniek tunneladres, bijvoorbeeld `10.77.0.2/32`;
- eigen `[Peer]`-blok op de server.

Deel nooit private keys of PSK's tussen gebruikers. Een gebruiker intrekken is
dan simpel: verwijder alleen diens `[Peer]`-blok en herlaad de configuratie.

## 1. Controleer adressen en poorten

```sh
ip -4 route
ip -6 route
sudo ss -lunp
sudo tailscale status
```

Het voorbeeld gebruikt `10.77.0.0/24` en UDP-poort `51820`. Kies andere waarden
als die al in gebruik zijn. Vermijd in het bijzonder overlap met Tailscale's
`100.64.0.0/10` en met lokale/container-netwerken.

## 2. Installeer WireGuard

```sh
sudo apt update
sudo apt install wireguard
sudo install -d -m 700 /etc/wireguard/keys
```

Maak de serversleutels aan zonder ze via stdout of shell-history te lekken:

```sh
sudo sh -c 'umask 077; wg genkey > /etc/wireguard/keys/server.key'
sudo sh -c 'wg pubkey < /etc/wireguard/keys/server.key > /etc/wireguard/keys/server.pub'
```

Maak per client lokaal een keypair. Alleen de public key gaat naar de server.
Maak de PSK op de server of client aan en draag hem via een reeds beveiligd
kanaal over:

```sh
umask 077
wg genkey > alice-laptop.key
wg pubkey < alice-laptop.key > alice-laptop.pub
wg genpsk > alice-laptop.psk
```

## 3. Serverconfiguratie

Plaats de inhoud van `server.key` bij `PrivateKey`, niet het pad naar het
bestand. Maak `/etc/wireguard/wg0.conf`:

```ini
[Interface]
Address = 10.77.0.1/24
ListenPort = 51820
PrivateKey = SERVER_PRIVATE_KEY

# alice-laptop
[Peer]
PublicKey = ALICE_PUBLIC_KEY
PresharedKey = ALICE_SERVER_PSK
AllowedIPs = 10.77.0.2/32

# bob-phone
[Peer]
PublicKey = BOB_PUBLIC_KEY
PresharedKey = BOB_SERVER_PSK
AllowedIPs = 10.77.0.3/32
```

Beveilig en valideer het bestand:

```sh
sudo chmod 600 /etc/wireguard/wg0.conf
sudo wg-quick strip wg0 >/dev/null
```

`AllowedIPs` op de server moet per peer een uniek `/32` zijn. Gebruik daar niet
`10.77.0.0/24` voor iedere peer: dan claimen meerdere peers dezelfde route.

## 4. Clientconfiguratie

Voor Alice, met alleen toegang tot de VPN-adressen van de server en andere
expliciet toegestane peers:

```ini
[Interface]
PrivateKey = ALICE_PRIVATE_KEY
Address = 10.77.0.2/32

[Peer]
PublicKey = SERVER_PUBLIC_KEY
PresharedKey = ALICE_SERVER_PSK
Endpoint = vpn.example.nl:51820
AllowedIPs = 10.77.0.0/24
PersistentKeepalive = 25
```

`PersistentKeepalive = 25` is nuttig voor een client achter NAT; de serverpeer
heeft dit normaal niet nodig. Laat `DNS` weg zolang je geen DNS-server via de
tunnel aanbiedt.

Voor uitsluitend toegang tot de server kan `AllowedIPs` op de client nog
strikter als `10.77.0.1/32`. Voeg later alleen de werkelijk benodigde
LAN-subnets toe. Gebruik `0.0.0.0/0, ::/0` uitsluitend als je bewust een
full-tunnel VPN wilt bouwen; dat vereist forwarding, NAT en passende IPv6-regels.

## 5. Firewall en starten

Als UFW actief is:

```sh
sudo ufw status verbose
sudo ufw allow 51820/udp comment 'WireGuard wg0'
```

Open daarnaast alleen diensten die VPN-gebruikers werkelijk mogen gebruiken.
Een voorbeeld voor SSH alleen vanaf het eigen WireGuard-subnet is:

```sh
sudo ufw allow in on wg0 from 10.77.0.0/24 to any port 22 proto tcp
```

Start de interface en schakel starten tijdens boot in:

```sh
sudo systemctl enable --now wg-quick@wg0
sudo systemctl status --no-pager wg-quick@wg0
sudo wg show wg0
```

Test vanaf de client eerst `ping 10.77.0.1` en daarna SSH naar
`10.77.0.1`. Controleer op de server of `latest handshake` en de bytecounters
veranderen:

```sh
sudo wg show wg0
```

## 6. Een peer toevoegen, wijzigen of intrekken

De duidelijkste beheerwijze is: wijzig `/etc/wireguard/wg0.conf`, valideer het
bestand en synchroniseer de actieve interface zonder die neer te halen:

```sh
sudo wg-quick strip wg0 | sudo wg syncconf wg0 /dev/stdin
sudo wg show wg0
```

Voor intrekken verwijder je het betreffende `[Peer]`-blok en voer je dezelfde
`syncconf`-opdracht uit. Verwijder of archiveer daarna de PSK volgens je
sleutelbeleid.

## Routing naar LAN of internet

Voor alleen toegang tot de server is geen IP-forwarding of NAT nodig. Voor
toegang tot een achterliggend LAN zijn aanvullend nodig:

1. het LAN-prefix in `AllowedIPs` van de client;
2. `net.ipv4.ip_forward=1` (en expliciet beleid voor IPv6 als dat gebruikt wordt);
3. firewall-forwardregels tussen `wg0` en de LAN-interface;
4. een retourroute naar `10.77.0.0/24` op de LAN-router, of als laatste keuze
   gerichte NAT/masquerading op de Ubuntu-server.

Gebruik bij voorkeur een echte retourroute in plaats van NAT: routing blijft
dan zichtbaar en bronadressen blijven behouden. Voeg geen brede `PostUp`/
`PostDown`-regels toe voordat interface, LAN-prefix en threat model bekend zijn.

## Tailscale naast `wg0`

- Laat `tailscaled` en `tailscale0` intact als beheer-/fallbackpad.
- Gebruik geen overlappend tunnelprefix en geen conflicterende UDP-listenpoort.
- Controleer routes na starten met `ip route` en `ip rule`.
- Publiceer hetzelfde LAN-prefix niet tegelijk via Tailscale subnet routing en
  `wg0` zonder bewust prioriteiten en retourroutes te ontwerpen.
- Gebruik firewallregels op interface (`wg0` versus `tailscale0`) om beide
  vertrouwensdomeinen van elkaar te scheiden.

## Grenzen van WireGuard-gebruikersbeheer

Een PSK maakt het protocol cryptografisch sterker naast de public-key handshake,
maar is geen rol- of rechtenmodel. WireGuard beslist welke peer welk tunnel-IP
mag gebruiken; UFW/nftables en de applicaties bepalen vervolgens welke diensten
die peer mag benaderen. Voor auditing leg je buiten de geheime config minimaal
vast: eigenaar, apparaat, public key, tunnel-IP, aanmaakdatum en intrekdatum.

## Officiële naslag

- [Ubuntu Server: WireGuard VPN](https://documentation.ubuntu.com/server/how-to/wireguard-vpn/)
- [WireGuard Quick Start](https://www.wireguard.com/quickstart/)
- [wg-quick manual](https://manpages.ubuntu.com/manpages/noble/en/man8/wg-quick.8.html)
- [Tailscale firewall ports](https://tailscale.com/kb/1082/firewall-ports)


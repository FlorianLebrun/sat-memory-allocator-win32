
$cert = New-SelfSignedCertificate -DnsName "Open Source" -Type CodeSigning -CertStoreLocation Cert:\CurrentUser\My
$CertPassword = ConvertTo-SecureString -String "my_password" -Force -AsPlainText
Export-PfxCertificate -Cert "cert:\CurrentUser\My\$($cert.Thumbprint)" -FilePath "$($PSScriptRoot)\build-certificate.pfx" -Password $CertPassword
